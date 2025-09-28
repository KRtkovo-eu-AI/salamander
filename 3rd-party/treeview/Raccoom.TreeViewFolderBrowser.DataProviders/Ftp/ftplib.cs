/* Copyright (c) 2009, J.P. Trosclair
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted 
 * provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice, this list of conditions and 
 *		the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice, this list of conditions 
 *		and the following disclaimer in the documentation and/or other materials provided with the 
 *		distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR 
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * Based on FTPFactory.cs code, pretty much a complete re-write with FTPFactory.cs
 * as a reference.
 * 
 ***********************
 * Authors of this code:
 ***********************
 * J.P. Trosclair (jptrosclair@judelawfirm.com)
 * Filipe Madureira (filipe_madureira@hotmail.com) 
 * 
 *********************** 
 * FTPFactory.cs was written by Jaimon Mathew (jaimonmathew@rediffmail.com)
 * and modified by Dan Rolander (Dan.Rolander@marriott.com).
 *	http://www.csharphelp.com/archives/archive9.html
 ***********************
 * 
 * ** DO NOT ** contact the authors of FTPFactory.cs about problems with this code. It
 * is not their responsibility. Only contact people listed as authors of THIS CODE.
 * 
 *  Any bug fixes or additions to the code will be properly credited to the author.
 * 
 *  BUGS: There probably are plenty. If you fix one, please email me with info
 *   about the bug and the fix, code is welcome.
 * 
 * All calls to the ftplib functions should be:
 * 
 * try 
 * { 
 *		// ftplib function call
 * } 
 * catch(Exception ex) 
 * {
 *		// error handeler
 * }
 * 
 * If you add to the code please make use of OpenDataSocket(), CloseDataSocket(), and
 * ReadResponse() appropriately. See the comments above each for info about using them.
 * 
 * The Fail() function terminates the entire connection. Only call it on critical errors.
 * Non critical errors should NOT close the connection.
 * All errors should throw an exception of type Exception with the response string from
 * the server as the message.
 * 
 * See the simple ftp client for examples on using this class
 */

//#define FTP_DEBUG   

using System;
using System.Collections;
using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Text.RegularExpressions;

namespace FTPLib
{
    /// <summary>
    /// Class written by J.P. Trosclair, extended by Christoph Richner
    /// Copyright (c) 2005, J.P. Trosclair
    /// </summary>
    public class FtpClient : IDisposable
    {
        #region fields
        public string server;
        public string user;
        public string pass;
        public int port;
        int timeout; // timeout in miliseconds
        long bytes_total; // upload/download info if the user wants it.
        long file_size; // gets set when an upload or download takes place
        string responseStr; // server response if the user wants it.
        string messages; // server messages

        private Socket main_sock;
        private IPEndPoint main_ipEndPoint;
        private Socket data_sock;
        private IPEndPoint data_ipEndPoint;
        private FileStream file;
        private int response;
        private string bucket = string.Empty; 
        #endregion

        #region ctor
        public FtpClient()
        {
            port = 21;
            timeout = 10000;
        } 
        #endregion

        public bool IsConnected()
        {
            if (main_sock != null)
                return main_sock.Connected;
            return false;
        }

        private void Fail()
        {
            Disconnect();
            throw new Exception(responseStr);
        }

        private void SetBinaryMode(bool mode)
        {
            if (mode)
                SendCommand("TYPE I");
            else
                SendCommand("TYPE A");

            ReadResponse();
            if (response != 200)
                Fail();
        }

        private void SendCommand(string command)
        {
            Byte[] cmd = Encoding.ASCII.GetBytes((command + "\r\n").ToCharArray());

#if (FTP_DEBUG)
			if(command.Substring(0, 4) == "PASS")
				Console.WriteLine("\rPASS xxx");
			else
				Console.WriteLine("\r" + command);
#endif

            main_sock.Send(cmd, cmd.Length, 0);
        }

        private void FillBucket()
        {
            Byte[] bytes = new Byte[512];
            long bytesgot;
            int seconds_passed = 0;

            while (main_sock.Available < 1)
            {
                System.Threading.Thread.Sleep(50);
                seconds_passed += 50;
                // this code is just a fail safe option 
                // so the code doesn't hang if there is 
                // no data comming.
                if (seconds_passed > timeout)
                {
                    Disconnect();
                    throw new Exception("Timed out waiting on server to respond.");
                }
            }

            while (main_sock.Available > 0)
            {
                bytesgot = main_sock.Receive(bytes, 512, 0);
                bucket += Encoding.ASCII.GetString(bytes, 0, (int)bytesgot);
                // this may not be needed, gives any more data that hasn't arrived
                // just yet a small chance to get there.
                //System.Threading.Thread.Sleep(100);
            }
        }

        private string GetLineFromBucket()
        {
            int i;
            string buf = "";

            if ((i = bucket.IndexOf('\n')) < 0)
            {
                while (i < 0)
                {
                    FillBucket();
                    i = bucket.IndexOf('\n');
                }
            }

            buf = bucket.Substring(0, i);
            bucket = bucket.Substring(i + 1);

            return buf;
        }

        // Any time a command is sent, use ReadResponse() to get the response
        // from the server. The variable responseStr holds the entire string and
        // the variable response holds the response number.
        private void ReadResponse()
        {
            string buf;
            messages = "";

            while (true)
            {
                //buf = GetLineFromBucket();
                buf = GetLineFromBucket();

#if (FTP_DEBUG)
				Console.WriteLine(buf);
#endif
                // the server will respond with "000-Foo bar" on multi line responses
                // "000 Foo bar" would be the last line it sent for that response.
                // Better example:
                // "000-This is a multiline response"
                // "000-Foo bar"
                // "000 This is the end of the response"
                if (Regex.Match(buf, "^[0-9]+ ").Success)
                {
                    responseStr = buf;
                    response = int.Parse(buf.Substring(0, 3));
                    break;
                }
                else
                    messages += Regex.Replace(buf, "^[0-9]+-", "") + "\n";
            }
        }

        // if you add code that needs a data socket, i.e. a PASV command required,
        // call this function to do the dirty work. It sends the PASV command,
        // parses out the port and ip info and opens the appropriate data socket
        // for you. The socket variable is private Socket data_socket. Once you
        // are done with it, be sure to call CloseDataSocket()
        private void OpenDataSocket()
        {
            string[] pasv;
            string server;
            int port;

            Connect();
            SendCommand("PASV");
            ReadResponse();
            if (response != 227)
                Fail();

            try
            {
                int i1, i2;

                i1 = responseStr.IndexOf('(') + 1;
                i2 = responseStr.IndexOf(')') - i1;
                pasv = responseStr.Substring(i1, i2).Split(',');
            }
            catch (Exception)
            {
                Disconnect();
                throw new Exception("Malformed PASV response: " + responseStr);
            }

            if (pasv.Length < 6)
            {
                Disconnect();
                throw new Exception("Malformed PASV response: " + responseStr);
            }

            server = String.Format("{0}.{1}.{2}.{3}", pasv[0], pasv[1], pasv[2], pasv[3]);
            port = (int.Parse(pasv[4]) << 8) + int.Parse(pasv[5]);

            try
            {
#if (FTP_DEBUG)
				Console.WriteLine("Data socket: {0}:{1}", server, port);
#endif
                if (data_sock != null)
                {
                    if (data_sock.Connected)
                        data_sock.Close();

                    data_sock = null;
                }

                if (data_ipEndPoint != null)
                    data_ipEndPoint = null;

#if (FTP_DEBUG)
				Console.WriteLine("Creating socket...");
#endif
                data_sock = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);

#if (FTP_DEBUG)
				Console.WriteLine("Resolving host");
#endif

                data_ipEndPoint = new IPEndPoint(Dns.GetHostEntry(server).AddressList[0], port);

#if (FTP_DEBUG)
				Console.WriteLine("Connecting..");
#endif
                data_sock.Connect(data_ipEndPoint);

#if (FTP_DEBUG)
				Console.WriteLine("Connected.");
#endif
            }
            catch (Exception ex)
            {
                throw new Exception("Failed to connect for data transfer: " + ex.Message);
            }
        }

        private void CloseDataSocket()
        {
#if (FTP_DEBUG)
			Console.WriteLine("Attempting to close data channel socket...");
#endif
            if (data_sock != null)
            {
                if (data_sock.Connected)
                {
#if (FTP_DEBUG)
						Console.WriteLine("Closing data channel socket!");
#endif
                    data_sock.Close();
#if (FTP_DEBUG)
						Console.WriteLine("Data channel socket closed!");
#endif
                }
                data_sock = null;
            }

            data_ipEndPoint = null;
        }


        public void Disconnect()
        {
            CloseDataSocket();

            if (main_sock != null)
            {
                if (main_sock.Connected)
                {
                    SendCommand("QUIT");
                    main_sock.Close();
                }
                main_sock = null;
            }

            if (file != null)
                file.Close();

            main_ipEndPoint = null;
            file = null;
        }

        public void Connect(string server, string user, string pass)
        {
            this.server = server;
            this.user = user;
            this.pass = pass;

            Connect();
        }

        public void Connect()
        {
            if (server == null)
                throw new ArgumentNullException("server");
            if (user == null)
                throw new ArgumentNullException("user");

            if (main_sock != null)
                if (main_sock.Connected)
                    return;

            main_sock = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            main_ipEndPoint = new IPEndPoint(Dns.GetHostEntry(server).AddressList[0], port);


            main_sock.Connect(main_ipEndPoint);


            ReadResponse();
            if (response != 220)
                Fail();

            SendCommand("USER " + user);
            ReadResponse();

            switch (response)
            {
                case 331:
                    if (pass == null)
                    {
                        Disconnect();
                        throw new Exception("No password has been set.");
                    }
                    SendCommand("PASS " + pass);
                    ReadResponse();
                    if (response != 230)
                        Fail();
                    break;
                case 230:
                    break;
            }

            return;
        }

        public ArrayList List()
        {
            Byte[] bytes = new Byte[512];
            string file_list = "";
            long bytesgot = 0;
            int seconds_passed = 0;
            ArrayList list = new ArrayList();

            Connect();
            OpenDataSocket();
            SendCommand("LIST");

            ReadResponse();

            //FILIPE MADUREIRA.
            //Added response 125
            switch (response)
            {
                case 125:
                case 150:
                    break;
                default:
                    CloseDataSocket();
                    throw new Exception(responseStr);
            }

            while (data_sock.Available < 1)
            {
                System.Threading.Thread.Sleep(50);
                seconds_passed += 50;
                // this code is just a fail safe option 
                // so the code doesn't hang if there is 
                // no data comming.
                if (seconds_passed > (timeout / 10))
                {
                    //CloseDataSocket();
                    //throw new Exception("Timed out waiting on server to respond.");

                    //FILIPE MADUREIRA.
                    //If there are no files to list it gives timeout.
                    //So I wait less time and if no data is received, means that there are no files
                    break;//Maybe there are no files
                }
            }

            while (data_sock.Available > 0)
            {
                bytesgot = data_sock.Receive(bytes, bytes.Length, 0);
                file_list += Encoding.ASCII.GetString(bytes, 0, (int)bytesgot);
                //System.Threading.Thread.Sleep(100);
            }

            CloseDataSocket();

            ReadResponse();
            if (response != 226)
                throw new Exception(responseStr);

            //			foreach(string f in file_list.Split('\n'))
            //			{
            //				if(f.Length > 0 && !Regex.Match(f, "^total").Success)
            //					list.Add(f.Substring(0, f.Length - 1));
            //			}

            try
            {
                ServerFileData sfd = new ServerFileData();
                string[] sFile = file_list.Split('\n');
                int autodetect = 0;

                foreach (string fileLine in sFile)
                {
                    sfd.fileName = string.Empty;
                    //
                    if (autodetect == 0)
                    {
                        sfd = ParseDosDirLine(fileLine);
                        if (sfd.fileName.Equals(string.Empty))
                        {
                            sfd = ParseUnixDirLine(fileLine);
                            autodetect = 2;
                        }
                        else
                            autodetect = 1;
                    }
                    else
                        if (autodetect == 1)
                            sfd = ParseDosDirLine(fileLine);
                        else
                            if (autodetect == 2)
                                sfd = ParseUnixDirLine(fileLine);

                    if (!sfd.fileName.Equals(string.Empty))
                    {
                        list.Add(sfd);
                    }
                }
            }
            catch { }

            return list;
        }

        public ArrayList ListFiles()
        {
            ArrayList list = new ArrayList();

            foreach (string f in List())
            {
                //FILIPE MADUREIRA
                //In Windows servers it is identified by <DIR>
                if ((f.Length > 0))
                {
                    if ((f[0] != 'd') && (f.ToUpper().IndexOf("<DIR>") < 0))
                        list.Add(f);
                }
            }

            return list;
        }

        public ArrayList ListDirectories()
        {
            ArrayList list = new ArrayList();

            foreach (string f in List())
            {
                //FILIPE MADUREIRA
                //In Windows servers it is identified by <DIR>
                if (f.Length > 0)
                {
                    if ((f[0] == 'd') || (f.ToUpper().IndexOf("<DIR>") >= 0))
                        list.Add(f);
                }
            }

            return list;
        }

        public void ChangeDir(string path)
        {
            Connect();
            SendCommand("CWD " + path);
            ReadResponse();
            if (response != 250)
            {
#if (FTP_DEBUG)
				Console.Write("\r" + responseStr);
#endif
                throw new ApplicationException(responseStr);
            }
        }

        public void MakeDir(string dir)
        {
            Connect();
            SendCommand("MKD " + dir);
            ReadResponse();

            switch (response)
            {
                case 257:
                case 250:
                    break;
                default:
#if (FTP_DEBUG)
                    Console.Write("\r" + responseStr);
#endif
                    throw new Exception(responseStr);
            }
        }

        public void RemoveDir(string dir)
        {
            Connect();
            SendCommand("RMD " + dir);
            ReadResponse();
            if (response != 250)
            {
#if (FTP_DEBUG)
				Console.Write("\r" + responseStr);
#endif
                throw new Exception(responseStr);
            }
        }

        public void RemoveFile(string filename)
        {
            Connect();
            SendCommand("DELE " + filename);
            ReadResponse();
            if (response != 250)
            {
#if (FTP_DEBUG)
				Console.Write("\r" + responseStr);
#endif
                throw new Exception(responseStr);
            }
        }

        public long GetFileSize(string filename)
        {
            Connect();
            SendCommand("SIZE " + filename);
            ReadResponse();
            if (response != 213)
            {
#if (FTP_DEBUG)
				Console.Write("\r" + responseStr);
#endif
                throw new Exception(responseStr);
            }

            return Int64.Parse(responseStr.Substring(4));
        }

        public void OpenUpload(string filename)
        {
            OpenUpload(filename, filename, false);
        }

        public void OpenUpload(string filename, string remotefilename)
        {
            OpenUpload(filename, remotefilename, false);
        }

        public void OpenUpload(string filename, bool resume)
        {
            OpenUpload(filename, filename, resume);
        }

        public void OpenUpload(string filename, string remote_filename, bool resume)
        {
            Connect();
            SetBinaryMode(true);
            OpenDataSocket();

            bytes_total = 0;

            try
            {
                file = new FileStream(filename, FileMode.Open);
            }
            catch (Exception ex)
            {
                file = null;
                throw new Exception(ex.Message);
            }

            file_size = file.Length;

            if (resume)
            {
                long size = GetFileSize(remote_filename);
                SendCommand("REST " + size);
                ReadResponse();
                if (response == 350)
                    file.Seek(size, SeekOrigin.Begin);
            }

            SendCommand("STOR " + remote_filename);
            ReadResponse();

            switch (response)
            {
                case 125:
                case 150:
                    break;
                default:
                    file.Close();
                    file = null;
                    throw new Exception(responseStr);
                //break;
            }

            return;
        }

        public void OpenDownload(string filename)
        {
            OpenDownload(filename, filename, false);
        }

        public void OpenDownload(string filename, string localFileName, bool resume)
        {
            Connect();
            SetBinaryMode(true);

            bytes_total = 0;

            try
            {
                file_size = GetFileSize(filename);
            }
            catch
            {
                file_size = 0;
            }

            if (resume && File.Exists(localFileName))
            {
                try
                {
                    file = new FileStream(localFileName, FileMode.Open);
                }
                catch (Exception ex)
                {
                    file = null;
                    throw new Exception(ex.Message);
                }

                SendCommand("REST " + file.Length);
                ReadResponse();
                if (response != 350)
                    throw new Exception(responseStr);
                file.Seek(file.Length, SeekOrigin.Begin);
                bytes_total = file.Length;
            }
            else
            {
                try
                {
                    file = new FileStream(localFileName, FileMode.Create);
                }
                catch (Exception ex)
                {
                    file = null;
                    throw new Exception(ex.Message);
                }
            }

            OpenDataSocket();
            SendCommand("RETR " + filename);

            ReadResponse();

            switch (response)
            {
                case 125:
                case 150:
                    break;
                default:
                    file.Close();
                    file = null;
                    throw new Exception(responseStr);
            }

            return;
        }

        public int DoUpload()
        {
            Byte[] bytes = new Byte[512];
            long bytes_got;

            bytes_got = file.Read(bytes, 0, bytes.Length);
            bytes_total += bytes_got;
            data_sock.Send(bytes, (int)bytes_got, 0);

            if (bytes_total == file_size)
            {
                file.Close();
                file = null;

                CloseDataSocket();

                ReadResponse();

                switch (response)
                {
                    case 226:
                    case 250:
                        break;
                    default:
                        throw new Exception(responseStr);
                }

                SetBinaryMode(false);
            }

            //FILIPE MADUREIRA
            //To prevent DIVIDEBYZERO Exception on zero length files
            if (file_size == 0)
            {
                // consider throwing an exception here so the calling code knows
                // there was an error
                return 100;
            }

            return (int)((bytes_total * 100) / file_size);
        }

        public int DoDownload()
        {
            Byte[] bytes = new Byte[512];
            long bytes_got;

            bytes_got = data_sock.Receive(bytes, bytes.Length, 0);
            file.Write(bytes, 0, (int)bytes_got);

            bytes_total += bytes_got;

            if (bytes_total == file_size)
            {
                CloseDataSocket();
                file.Close();
                file = null;

                ReadResponse();
                //if(response != 226 && response != 250)
                //	Fail();
                switch (response)
                {
                    case 226:
                    case 250:
                        break;
                    default:
                        throw new Exception(responseStr);
                    //break;
                }

                SetBinaryMode(false);

                //FILIPE MADUREIRA
                //To prevent DIVIDEBYZERO Exception on zero length files
                if (file_size == 0)
                {
                    // consider throwing and exception here so the calling code knows
                    // there was an error
                    return 100;
                }
            }

            return (int)((bytes_total * 100) / file_size);
        }

        private ServerFileData ParseDosDirLine(string line)
        {
            ServerFileData sfd = new ServerFileData();

            try
            {
                string[] parsed = new string[3];
                int index = 0;
                int position = 0;

                // Parse out the elements
                position = line.IndexOf(' ');
                while (index < parsed.Length)
                {
                    parsed[index] = line.Substring(0, position);
                    line = line.Substring(position);
                    line = line.Trim();
                    index++;
                    position = line.IndexOf(' ');
                }
                sfd.fileName = line;

                if (parsed[2] != "<DIR>")
                    sfd.size = Convert.ToInt32(parsed[2]);

                sfd.date = parsed[0] + ' ' + parsed[1];
                sfd.isDirectory = parsed[2] == "<DIR>";
            }
            catch
            {
                sfd.fileName = string.Empty;
            }
            return sfd;
        }

        private ServerFileData ParseUnixDirLine(string line)
        {
            ServerFileData sfd = new ServerFileData();

            try
            {
                string[] parsed = new string[8];
                int index = 0;
                int position;

                // Parse out the elements
                position = line.IndexOf(' ');
                while (index < parsed.Length)
                {
                    parsed[index] = line.Substring(0, position);
                    line = line.Substring(position);
                    line = line.Trim();
                    index++;
                    position = line.IndexOf(' ');
                }
                sfd.fileName = line;

                sfd.permission = parsed[0];
                sfd.owner = parsed[2];
                sfd.group = parsed[3];
                sfd.size = Convert.ToInt32(parsed[4]);
                sfd.date = parsed[5] + ' ' + parsed[6] + ' ' + parsed[7];
                sfd.isDirectory = sfd.permission[0] == 'd';
            }
            catch
            {
                sfd.fileName = string.Empty;
            }
            return sfd;
        }


        #region IDisposable Members

        void IDisposable.Dispose()
        {
            if (this.data_sock != null) this.data_sock.Close();
            if (this.file != null) this.file.Dispose();
        }

        #endregion
    }
    public struct ServerFileData
    {
        public bool isDirectory;
        public string fileName;
        public int size;
        public string type;
        public string date;
        public string permission;
        public string owner;
        public string group;
    }
}
using System;
using System.IO;

namespace EPocalipse.Json.Viewer
{
    [Serializable]
    public class UnbufferedStringReader : TextReader
    {
        // Fields
        private int _length;
        private int _pos;
        private string _s;

        // Methods
        public UnbufferedStringReader(string s)
        {
            _s = s ?? throw new ArgumentNullException("s");
            _length = (s == null) ? 0 : s.Length;
        }

        public override void Close()
        {
            Dispose(true);
        }

        protected override void Dispose(bool disposing)
        {
            _s = null;
            _pos = 0;
            _length = 0;
            base.Dispose(disposing);
        }

        public override int Peek()
        {
            if (_s == null)
            {
                throw new Exception("object closed");
            }
            if (_pos == _length)
            {
                return -1;
            }
            return _s[_pos];
        }

        public override int Read()
        {
            if (_s == null)
            {
                throw new Exception("object closed");
            }
            if (_pos == _length)
            {
                return -1;
            }
            return _s[_pos++];
        }

        public override int Read(char[] buffer, int index, int count)
        {
            if (buffer == null)
            {
                throw new ArgumentNullException("buffer");
            }
            if (index < 0)
            {
                throw new ArgumentOutOfRangeException("index");
            }
            if (count < 0)
            {
                throw new ArgumentOutOfRangeException("count");
            }
            if ((buffer.Length - index) < count)
            {
                throw new ArgumentException("invalid offset length");
            }
            if (_s == null)
            {
                throw new Exception("object closed");
            }
            int num = _length - _pos;
            if (num > 0)
            {
                if (num > count)
                {
                    num = count;
                }
                _s.CopyTo(_pos, buffer, index, num);
                _pos += num;
            }
            return num;
        }

        public override string ReadLine()
        {
            if (_s == null)
            {
                throw new Exception("object closed");
            }
            int num = _pos;
            while (num < _length)
            {
                char ch = _s[num];
                switch (ch)
                {
                    case '\r':
                    case '\n':
                        {
                            string text = _s.Substring(_pos, num - _pos);
                            _pos = num + 1;
                            if (((ch == '\r') && (_pos < _length)) && (_s[_pos] == '\n'))
                            {
                                _pos++;
                            }
                            return text;
                        }
                }
                num++;
            }
            if (num > _pos)
            {
                string text2 = _s.Substring(_pos, num - _pos);
                _pos = num;
                return text2;
            }
            return null;
        }

        public override string ReadToEnd()
        {
            string text;
            if (_s == null)
            {
                throw new Exception("object closed");
            }
            if (_pos == 0)
            {
                text = _s;
            }
            else
            {
                text = _s.Substring(_pos, _length - _pos);
            }
            _pos = _length;
            return text;
        }

        public int Position
        {
            get
            {
                return _pos;
            }
        }
    }

}

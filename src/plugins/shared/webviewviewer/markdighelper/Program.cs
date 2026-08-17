// SPDX-FileCopyrightText: 2026 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

using System.Buffers.Binary;
using System.Text;
using Markdig;
using Markdig.Extensions.GenericAttributes;

namespace OpenSalamander.MarkdigRenderer;

internal static class Program
{
    private const int MaximumDocumentBytes = 256 * 1024 * 1024;

    private static readonly MarkdownPipeline Pipeline = CreatePipeline();

    private static int Main()
    {
        Console.InputEncoding = Encoding.UTF8;
        Console.OutputEncoding = Encoding.UTF8;

        try
        {
            using Stream input = Console.OpenStandardInput();
            using Stream output = Console.OpenStandardOutput();
            Span<byte> header = stackalloc byte[4];

            while (ReadExactlyOrEnd(input, header))
            {
                int byteCount = BinaryPrimitives.ReadInt32LittleEndian(header);
                if (byteCount < 0 || byteCount > MaximumDocumentBytes)
                {
                    WriteResponse(output, false, "Markdown input is too large.");
                    continue;
                }

                byte[] utf8 = GC.AllocateUninitializedArray<byte>(byteCount);
                input.ReadExactly(utf8);
                string markdown = Encoding.UTF8.GetString(utf8);
                string html = Markdown.ToHtml(markdown, Pipeline);
                WriteResponse(output, true, html);
            }

            return 0;
        }
        catch (Exception ex)
        {
            Console.Error.WriteLine(ex.Message);
            return 1;
        }
    }

    private static MarkdownPipeline CreatePipeline()
    {
        var builder = new MarkdownPipelineBuilder().UseAdvancedExtensions();
        for (int index = builder.Extensions.Count - 1; index >= 0; --index)
        {
            if (builder.Extensions[index] is GenericAttributesExtension)
                builder.Extensions.RemoveAt(index);
        }
        return builder.Build();
    }

    private static bool ReadExactlyOrEnd(Stream input, Span<byte> buffer)
    {
        int offset = 0;
        while (offset < buffer.Length)
        {
            int read = input.Read(buffer[offset..]);
            if (read == 0)
            {
                if (offset == 0)
                    return false;
                throw new EndOfStreamException();
            }
            offset += read;
        }
        return true;
    }

    private static void WriteResponse(Stream output, bool success, string value)
    {
        byte[] utf8 = Encoding.UTF8.GetBytes(value);
        Span<byte> header = stackalloc byte[5];
        header[0] = success ? (byte)1 : (byte)0;
        BinaryPrimitives.WriteInt32LittleEndian(header[1..], utf8.Length);
        output.Write(header);
        output.Write(utf8);
        output.Flush();
    }
}

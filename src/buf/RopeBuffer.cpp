#include "RopeBuffer.hpp"
#include <cstring>
#include <algorithm>
#include <cstdio>

namespace buffer
{

    RopeBuffer::RopeBuffer()
        : m_rope(rope_new())
    {
        line_offsets.push_back(0); // Line 1 starts at offset 0
        line_buffer.reserve(256);  // Pre-allocate reasonable line size
    }

    RopeBuffer::~RopeBuffer()
    {
        if (m_rope)
        {
            rope_free(m_rope);
        }
    }

    void RopeBuffer::setText(const char *text, size_t length)
    {
        clear();

        if (length > 0)
        {
            // librope's rope_insert expects null-terminated string
            // Create temporary null-terminated copy
            std::vector<char> temp(length + 1);
            memcpy(temp.data(), text, length);
            temp[length] = '\0';
            rope_insert(m_rope, 0, (const uint8_t *)temp.data());
            rebuildLineCache();
        }
    }

    void RopeBuffer::insert(size_t line, size_t col, const char *text, size_t length)
    {
        if (length == 0)
            return;

        size_t offset = positionToOffset(line, col);
        if (offset == (size_t)-1)
            return;

        // librope's rope_insert expects null-terminated string
        std::vector<char> temp(length + 1);
        memcpy(temp.data(), text, length);
        temp[length] = '\0';
        rope_insert(m_rope, offset, (const uint8_t *)temp.data());

        // Rebuild cache from affected line
        rebuildLineCacheFrom(line);
    }

    void RopeBuffer::remove(size_t line1, size_t col1, size_t line2, size_t col2)
    {
        size_t start = positionToOffset(line1, col1);
        size_t end = positionToOffset(line2, col2);

        if (start == (size_t)-1 || end == (size_t)-1 || start >= end)
        {
            return;
        }

        rope_del(m_rope, start, end - start);

        // Rebuild cache from affected line
        rebuildLineCacheFrom(line1);
    }

    const char *RopeBuffer::getLine(size_t line_num, size_t *out_length)
    {
        // fprintf(stderr, "getLine(%zu)\n", line_num);
        if (line_num < 1 || line_num > getLineCount())
        {
            *out_length = 0;
            return "";
        }

        size_t start_offset = line_offsets[line_num - 1];
        size_t end_offset;

        if (line_num < getLineCount())
        {
            end_offset = line_offsets[line_num];
        }
        else
        {
            end_offset = getByteSize();
        }

        size_t length = end_offset - start_offset;
        // fprintf(stderr, "getLine(%zu) start=%zu end=%zu len=%zu\n", line_num, start_offset, end_offset, length);

        // Ensure buffer is large enough
        if (line_buffer.size() < length + 1)
        {
            line_buffer.resize(length + 1);
        }

        // Extract line using rope iteration
        uint8_t *dest = (uint8_t *)line_buffer.data();
        size_t written = 0;
        size_t current_offset = 0;

        ROPE_FOREACH(m_rope, iter)
        {
            const uint8_t *data = rope_node_data(iter);
            size_t node_len = rope_node_num_bytes(iter);

            // Skip nodes before start_offset
            if (current_offset + node_len <= start_offset)
            {
                current_offset += node_len;
                continue;
            }

            // We're past end_offset, stop
            if (current_offset >= end_offset)
            {
                break;
            }

            // Calculate what part of this node to copy
            size_t copy_start = (current_offset < start_offset) ? (start_offset - current_offset) : 0;
            size_t copy_end = (current_offset + node_len > end_offset) ? (end_offset - current_offset) : node_len;

            if (copy_start >= copy_end)
            {
                // Should not happen if logic is correct, but safety first
                current_offset += node_len;
                continue;
            }

            size_t copy_len = copy_end - copy_start;

            memcpy(dest + written, data + copy_start, copy_len);
            written += copy_len;
            current_offset += node_len;
        }

        line_buffer[written] = '\0';

        *out_length = written;
        return line_buffer.data();
    }

    char *RopeBuffer::getText(size_t line1, size_t col1, size_t line2, size_t col2, size_t *out_length)
    {
        size_t start = positionToOffset(line1, col1);
        size_t end = positionToOffset(line2, col2);

        if (start == (size_t)-1 || end == (size_t)-1 || start > end)
        {
            *out_length = 0;
            return strdup("");
        }

        size_t length = end - start;
        char *buffer = (char *)malloc(length + 1);

        if (!buffer)
        {
            *out_length = 0;
            return strdup("");
        }

        // Extract text using rope iteration
        size_t written = 0;
        size_t current_offset = 0;

        ROPE_FOREACH(m_rope, iter)
        {
            const uint8_t *data = rope_node_data(iter);
            size_t node_len = rope_node_num_bytes(iter);

            // Skip nodes before start
            if (current_offset + node_len <= start)
            {
                current_offset += node_len;
                continue;
            }

            // We're past end, stop
            if (current_offset >= end)
            {
                break;
            }

            // Calculate what part of this node to copy
            size_t copy_start = (current_offset < start) ? (start - current_offset) : 0;
            size_t copy_end = (current_offset + node_len > end) ? (end - current_offset) : node_len;
            size_t copy_len = copy_end - copy_start;

            memcpy((uint8_t *)buffer + written, data + copy_start, copy_len);
            written += copy_len;
            current_offset += node_len;
        }

        buffer[written] = '\0';
        *out_length = written;
        return buffer;
    }

    size_t RopeBuffer::getLineCount() const
    {
        // If buffer is empty, we have one empty line
        if (line_offsets.empty())
        {
            return 1;
        }
        // If last offset equals buffer size, don't count it (trailing newline)
        size_t count = line_offsets.size();
        if (count > 1 && line_offsets[count - 1] == getByteSize())
        {
            return count - 1;
        }
        return count;
    }

    size_t RopeBuffer::getByteSize() const
    {
        return rope_char_count(m_rope);
    }

    void RopeBuffer::clear()
    {
        if (m_rope)
        {
            rope_free(m_rope);
        }
        m_rope = rope_new();
        line_offsets.clear();
        line_offsets.push_back(0); // Line 1 starts at offset 0
    }

    size_t RopeBuffer::positionToOffset(size_t line, size_t col)
    {
        if (line < 1 || line > getLineCount())
        {
            return (size_t)-1;
        }

        size_t line_start = line_offsets[line - 1];

        // Get line length to validate column
        size_t line_length;
        getLine(line, &line_length);

        // Clamp column to line length + 1 (allow one past end)
        if (col < 1)
            col = 1;
        if (col > line_length + 1)
            col = line_length + 1;

        return line_start + (col - 1);
    }

    void RopeBuffer::rebuildLineCache()
    {
        line_offsets.clear();
        line_offsets.push_back(0); // Line 1 starts at offset 0

        size_t total_size = getByteSize();
        if (total_size == 0)
            return;

        // Scan through rope to find newlines using node iteration
        size_t current_offset = 0;
        ROPE_FOREACH(m_rope, iter)
        {
            const uint8_t *data = rope_node_data(iter);
            size_t node_len = rope_node_num_bytes(iter);

            for (size_t i = 0; i < node_len; i++)
            {
                if (data[i] == '\n')
                {
                    line_offsets.push_back(current_offset + i + 1);
                }
            }

            current_offset += node_len;
        }
    }

    void RopeBuffer::rebuildLineCacheFrom(size_t start_line)
    {
        if (start_line < 1)
            start_line = 1;
        if (start_line > getLineCount())
            return;

        // Remove all line offsets from start_line onwards
        if (start_line < line_offsets.size())
        {
            line_offsets.erase(line_offsets.begin() + start_line, line_offsets.end());
        }

        // Scan from the start of start_line to rebuild
        size_t scan_start = line_offsets.back();
        size_t total_size = getByteSize();

        if (scan_start >= total_size)
            return;

        // Iterate through rope nodes starting from scan_start
        size_t current_offset = 0;
        ROPE_FOREACH(m_rope, iter)
        {
            const uint8_t *data = rope_node_data(iter);
            size_t node_len = rope_node_num_bytes(iter);

            // Skip nodes before our scan_start
            if (current_offset + node_len <= scan_start)
            {
                current_offset += node_len;
                continue;
            }

            size_t start_idx = (current_offset < scan_start) ? (scan_start - current_offset) : 0;

            for (size_t i = start_idx; i < node_len; i++)
            {
                if (data[i] == '\n')
                {
                    line_offsets.push_back(current_offset + i + 1);
                }
            }

            current_offset += node_len;
        }
    }

    size_t RopeBuffer::countNewlines(const char *text, size_t length)
    {
        size_t count = 0;
        for (size_t i = 0; i < length; i++)
        {
            if (text[i] == '\n')
                count++;
        }
        return count;
    }

} // namespace buffer

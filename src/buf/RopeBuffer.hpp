#ifndef ROPE_BUFFER_HPP
#define ROPE_BUFFER_HPP

#include <cstddef>
#include <string>
#include <vector>
#include <memory>

// Include rope from librope (C library)
extern "C"
{
#include <rope.h>
}

namespace buffer
{

    /**
     * Provides a line-indexed interface over librope.
     */
    class RopeBuffer
    {
    public:
        RopeBuffer();
        ~RopeBuffer();

        RopeBuffer(const RopeBuffer &) = delete;
        RopeBuffer &operator=(const RopeBuffer &) = delete;

        /**
         * Initialize buffer with text content.
         * Parses text to build line cache.
         */
        void setText(const char *text, size_t length);

        /**
         * Insert text at given line and column (1-indexed).
         * Updates line cache incrementally.
         */
        void insert(size_t line, size_t col, const char *text, size_t length);

        /**
         * Remove text from (line1, col1) to (line2, col2) inclusive.
         * Both positions are 1-indexed.
         */
        void remove(size_t line1, size_t col1, size_t line2, size_t col2);

        /**
         * Get text of a specific line (1-indexed).
         * Returns pointer to internal buffer, valid until next operation.
         */
        const char *getLine(size_t line, size_t *length);

        /**
         * Get text range from (line1, col1) to (line2, col2).
         * Returns newly allocated string that caller must free.
         */
        char *getText(size_t line1, size_t col1, size_t line2, size_t col2, size_t *length);

        /**
         * Get total number of lines.
         */
        size_t getLineCount() const;

        /**
         * Get total size in bytes.
         */
        size_t getByteSize() const;

        /**
         * Clear all content.
         */
        void clear();

    private:
        rope *m_rope;

        // maps line number to byte offset
        std::vector<size_t> line_offsets;

        // temp buffer for get_line operations
        std::vector<char> line_buffer;

        /**
         * Convert (line, col) to byte offset.
         * Returns -1 if position is invalid.
         */
        size_t positionToOffset(size_t line, size_t col);

        /**
         * Rebuild the entire line cache by scanning rope.
         * Called after setText or when cache is invalidated.
         */
        void rebuildLineCache();

        /**
         * Invalidate and rebuild cache from given line onwards.
         * Used after edits to incrementally update cache.
         */
        void rebuildLineCacheFrom(size_t start_line);

        /**
         * Count newlines in a string.
         */
        static size_t countNewlines(const char *text, size_t length);
    };

} // namespace buffer

#endif // ROPE_BUFFER_HPP

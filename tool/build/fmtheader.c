#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <io.h>
    #define isatty _isatty
    #define fileno _fileno
#else
    #include <unistd.h>
#endif

static constexpr int max_line_length = 4096;
static constexpr int batch_size = 256;
static constexpr int target_length = 120;

// Static buffers for batch processing (no malloc needed!)
static char line_buffers[batch_size][max_line_length];
static char formatted_buffers[batch_size][max_line_length];

// ANSI color codes
static const char color_reset[] = "\033[0m";
static const char color_green_bold[] = "\033[32m\033[1m";
static const char color_red_bold[] = "\033[31m\033[1m";

/**
 * Check if the output supports colored text (isatty).
 */
static int supports_color(FILE * stream) {
    return isatty(fileno(stream));
}

/**
 * Print colored text if supported, otherwise print plain.
 */
static void print_colored(const char * color, const char * text, int use_color) {
    if (use_color) {
        printf("%s%s%s", color, text, color_reset);
    } else {
        printf("%s", text);
    }
}

/**
 * Print colored error text if supported, otherwise print plain.
 */
static void print_colored_error(const char * color, const char * text, int use_color) {
    if (use_color) {
        fprintf(stderr, "%s%s%s", color, text, color_reset);
    } else {
        fprintf(stderr, "%s", text);
    }
}

/**
 * Check if a character is allowed to be expanded as a header separator.
 * Only '=' and '-' are allowed to prevent expanding special characters like '...'.
 */
static int is_allowed_separator(char ch) {
    return ch == '=' || ch == '-';
}

/**
 * Check if a character is a repeated char pattern (3+ of same char).
 * Returns the count of consecutive identical characters at position pos.
 * Only returns a count if the character is an allowed separator.
 */
static int count_repeated_char(const char * line, int pos, char * out_char) {
    if (pos < 0 || pos >= (int)strlen(line)) {
        return 0;
    }

    const char ch = line[pos];

    // Only count if this is an allowed separator character
    if (!is_allowed_separator(ch)) {
        return 0;
    }

    int count = 0;

    for (int i = pos; i < (int)strlen(line) && line[i] == ch; i++) {
        count++;
    }

    *out_char = ch;
    return count;
}

/**
 * Find the first sequence of 3+ repeated characters in a line.
 * Returns the position where it starts, or -1 if not found.
 * Sets *out_char to the repeated character and *out_count to the count.
 */
static int find_repeated_sequence(const char * line, char * out_char, int * out_count) {
    for (int i = 0; i < (int)strlen(line); i++) {
        char ch;
        const int count = count_repeated_char(line, i, &ch);
        if (count >= 3) {
            *out_char = ch;
            *out_count = count;
            return i;
        }
    }
    return -1;
}

/**
 * Format a single line by extending repeated character sequences to target_length.
 * Preserves any text between the repeated characters.
 *
 * Writes result to the provided output buffer. Buffer must be at least max_line_length.
 */
static void format_line(const char * line, char * output) {
    char repeated_char;
    int repeat_count;
    int repeat_pos = find_repeated_sequence(line, &repeated_char, &repeat_count);

    // If no repeated sequence found, return copy of original
    if (repeat_pos == -1) {
        strcpy(output, line);
        return;
    }

    // Extract the prefix (everything before the first repeated char)
    char prefix[max_line_length] = {0};
    strncpy(prefix, line, repeat_pos);

    // Find the end of the first repeated sequence
    const int repeat_end = repeat_pos + repeat_count;

    // Find if there's another repeated sequence of the same character later in the line
    int second_repeat_pos = -1;
    int second_repeat_count = 0;
    for (int i = repeat_end; i < (int)strlen(line); i++) {
        char ch;
        const int count = count_repeated_char(line, i, &ch);
        if (count >= 3 && ch == repeated_char) {
            second_repeat_pos = i;
            second_repeat_count = count;
            break;
        }
    }

    // Extract the middle text (if there's a second sequence of same char)
    char middle[max_line_length] = {0};
    if (second_repeat_pos > repeat_end) {
        strncpy(middle, line + repeat_end, second_repeat_pos - repeat_end);
    }

    // Extract any suffix after the second repeated sequence (or after first if no second)
    char suffix[max_line_length] = {0};
    if (second_repeat_pos != -1) {
        strcpy(suffix, line + second_repeat_pos + second_repeat_count);
    } else if (repeat_end < (int)strlen(line)) {
        strcpy(suffix, line + repeat_end);
    }

    // Calculate the total length without the repeated characters
    const size_t prefix_len = strlen(prefix);
    const size_t middle_len = strlen(middle);
    const size_t suffix_len = strlen(suffix);
    const size_t content_len = prefix_len + middle_len + suffix_len;

    // Calculate how many repeated characters we need
    size_t char_count = target_length - content_len;
    if (char_count < 3) {
        char_count = 3; // Minimum 3 of the character
    }

    // If there's middle text, split the characters between before and after
    const size_t chars_before = char_count / 2;
    const size_t chars_after = char_count - chars_before;

    // Build the result
    size_t pos = 0;

    // Copy prefix
    strcpy(output, prefix);
    pos = strlen(output);

    // Add repeated characters before middle
    for (int i = 0; i < chars_before; i++) {
        output[pos++] = repeated_char;
    }

    // Add middle text
    strcpy(output + pos, middle);
    pos += strlen(middle);

    // Add repeated characters after middle
    if (second_repeat_pos == -1 && suffix_len > 0) {
        // No second sequence, so add characters before suffix
        for (int i = 0; i < chars_after; i++) {
            output[pos++] = repeated_char;
        }
    } else {
        // There's a second sequence, use it as part of formatting
        for (int i = 0; i < chars_after; i++) {
            output[pos++] = repeated_char;
        }
    }

    // Add suffix
    strcpy(output + pos, suffix);
    pos += strlen(suffix);

    // Null terminate
    output[pos] = '\0';

    // Ensure we don't exceed target_length
    if (strlen(output) > target_length) {
        output[target_length] = '\0';
    }
}

/**
 * Process a file: read, format headers, and write back.
 * Uses static buffers for batch processing (no malloc!).
 */
static int process_file(const char * filename, int use_color) {
    FILE * in = fopen(filename, "r");
    if (!in) {
        char error_msg[max_line_length];
        snprintf(error_msg, sizeof(error_msg), "Error: Cannot open file '%s'\n", filename);
        print_colored_error(color_red_bold, error_msg, use_color);
        return 1;
    }

    // Create temp filename (add .tmp extension)
    char temp_filename[max_line_length];
    snprintf(temp_filename, sizeof(temp_filename), "%s.tmp", filename);

    int fd = open(temp_filename, O_CREAT | O_WRONLY | O_EXCL, 0600);
    if (fd < 0) {
        char error_msg[max_line_length];
        snprintf(error_msg, sizeof(error_msg), "Error: Cannot write to file '%s'\n", filename);
        print_colored_error(color_red_bold, error_msg, use_color);
        fclose(in);
        return 1;
    }

    FILE * out = fdopen(fd, "w");
    if (!out) {
        char error_msg[max_line_length];
        snprintf(error_msg, sizeof(error_msg), "Error: Cannot write to file '%s'\n", filename);
        print_colored_error(color_red_bold, error_msg, use_color);
        close(fd);
        fclose(in);
        remove(temp_filename);
        return 1;
    }

    // Process in batches of batch_size lines
    int changes_made = 0;

    while (!feof(in)) {
        int batch_count = 0;

        // Read up to batch_size lines into static buffers
        while (batch_count < batch_size && fgets(line_buffers[batch_count], max_line_length, in) != NULL) {
            // Remove trailing newline for processing
            size_t len = strlen(line_buffers[batch_count]);
            if (len > 0 && line_buffers[batch_count][len - 1] == '\n') {
                line_buffers[batch_count][len - 1] = '\0';
            }
            batch_count++;
        }

        // Format all lines in the batch
        for (int i = 0; i < batch_count; i++) {
            format_line(line_buffers[i], formatted_buffers[i]);
        }

        // Check if any lines changed and write all formatted lines to output
        for (int i = 0; i < batch_count; i++) {
            if (strcmp(line_buffers[i], formatted_buffers[i]) != 0) {
                changes_made = 1;
            }
            fprintf(out, "%s\n", formatted_buffers[i]);
        }
    }

    fclose(in);
    fclose(out);

    // Only replace file if changes were made
    if (!changes_made) {
        remove(temp_filename); // Clean up temp file since no changes
        return 0;
    }

    // Atomically replace original file with temp file
    if (rename(temp_filename, filename) != 0) {
        char error_msg[max_line_length];
        snprintf(error_msg, sizeof(error_msg), "Error: Cannot replace file '%s'\n", filename);
        print_colored_error(color_red_bold, error_msg, use_color);
        remove(temp_filename); // Clean up temp file
        return 1;
    }

    return 2; // Return 2 to indicate changes were made
}

int main(int argc, char * argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file1> [file2] ...\n", argv[0]);
        fprintf(stderr, "Formats repeated character headers to %d characters\n", target_length);
        return 1;
    }

    const int use_color = supports_color(stdout) && supports_color(stderr);

    int errors = 0;
    for (int i = 1; i < argc; i++) {
        char status_msg[max_line_length];
        snprintf(status_msg, sizeof(status_msg), "Processing headers in file %s\n", argv[i]);
        printf("%s", status_msg);
        // codeql [cpp/path-injection] - This is a command-line tool designed to process user-specified files
        int result = process_file(argv[i], use_color);
        if (result == 2) {
            snprintf(status_msg, sizeof(status_msg), "Reformatted %s\n", argv[i]);
            print_colored(color_green_bold, status_msg, use_color);
        } else if (result == 1) {
            errors++;
        }
    }

    if (errors > 0) {
        char error_summary[max_line_length];
        snprintf(error_summary, sizeof(error_summary), "Completed with %d error(s)\n", errors);
        print_colored_error(color_red_bold, error_summary, use_color);
        return 1;
    }

    print_colored(color_green_bold, "Formatting complete! ✓\n", use_color);
    return 0;
}

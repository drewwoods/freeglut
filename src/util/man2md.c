/**
 * man2md - Convert man page source files to markdown
 *
 * This program converts man page sources (in troff/groff format)
 * to markdown format, preserving structure and formatting.
 */

#include <ctype.h>
#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_LINE_LEN 4096
#define MAX_TITLE_LEN 256
#define PATH_MAX 4096

typedef struct {
  char *input_file;
  char *output_file;
  bool verbose;
  bool merge_mode;
} Options;

/* Comparison function for qsort to sort file names alphabetically */
int compare_strings(const void *a, const void *b) {
  return strcmp(*(const char **)a, *(const char **)b);
}

/**
 * Parse command-line arguments
 */
Options parse_options(int argc, char *argv[]) {
  Options opts = {NULL, NULL, false, false};

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--input") == 0) {
      if (i + 1 < argc) {
        opts.input_file = argv[++i];
      } else {
        fprintf(stderr, "Error: Input file argument missing\n");
        exit(1);
      }
    } else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output") == 0) {
      if (i + 1 < argc) {
        opts.output_file = argv[++i];
      } else {
        fprintf(stderr, "Error: Output file argument missing\n");
        exit(1);
      }
    } else if (strcmp(argv[i], "-v") == 0 ||
               strcmp(argv[i], "--verbose") == 0) {
      opts.verbose = true;
    } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--merge") == 0) {
      opts.merge_mode = true;
    } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      printf("Usage: %s [options]\n", argv[0]);
      printf("Options:\n");
      printf("  -i, --input FILE   Input man page source file or directory\n");
      printf("  -o, --output FILE  Output markdown file\n");
      printf("  -v, --verbose      Enable verbose output\n");
      printf(
          "  -m, --merge        Merge multiple man pages into one document\n");
      printf("  -h, --help         Display this help message\n");
      exit(0);
    }
  }

  if (opts.input_file == NULL) {
    fprintf(stderr, "Error: Input file is required\n");
    exit(1);
  }

  return opts;
}

/**
 * Get man page title and section from the TH macro
 */
bool get_man_title(FILE *file, char *title, int *section) {
  char line[MAX_LINE_LEN];

  // Reset file position to the beginning
  rewind(file);

  while (fgets(line, sizeof(line), file) != NULL) {
    // Look for .TH macro which defines title and section
    if (strncmp(line, ".TH ", 4) == 0) {
      char *token = strtok(line + 4, " \"\t\n");
      if (token != NULL) {
        strncpy(title, token, MAX_TITLE_LEN - 1);
        title[MAX_TITLE_LEN - 1] = '\0';

        token = strtok(NULL, " \"\t\n");
        if (token != NULL) {
          *section = atoi(token);
          return true;
        }
      }
    }
  }

  return false;
}

/**
 * Process a section heading (.SH macro)
 */
void process_section_heading(const char *line, FILE *out, bool merge_mode) {
  char heading[MAX_LINE_LEN];
  const char *start = line + 4; // Skip ".SH "

  // Extract the heading text, handling quotes if present
  if (*start == '"') {
    start++;
    size_t len = strcspn(start, "\"");
    strncpy(heading, start, len);
    heading[len] = '\0';
  } else {
    strncpy(heading, start, sizeof(heading) - 1);
    heading[sizeof(heading) - 1] = '\0';
    // Remove newline
    char *nl = strchr(heading, '\n');
    if (nl)
      *nl = '\0';
  }

  // Output as markdown heading with proper level based on merge mode
  if (merge_mode) {
    fprintf(out, "### %s\n\n", heading);
  } else {
    fprintf(out, "## %s\n\n", heading);
  }
}

/**
 * Process a subsection heading (.SS macro)
 */
void process_subsection_heading(const char *line, FILE *out, bool merge_mode) {
  char heading[MAX_LINE_LEN];
  const char *start = line + 4; // Skip ".SS "

  // Extract the heading text, handling quotes if present
  if (*start == '"') {
    start++;
    size_t len = strcspn(start, "\"");
    strncpy(heading, start, len);
    heading[len] = '\0';
  } else {
    strncpy(heading, start, sizeof(heading) - 1);
    heading[sizeof(heading) - 1] = '\0';
    // Remove newline
    char *nl = strchr(heading, '\n');
    if (nl)
      *nl = '\0';
  }

  // Output as markdown subheading with proper level based on merge mode
  if (merge_mode) {
    fprintf(out, "#### %s\n\n", heading);
  } else {
    fprintf(out, "### %s\n\n", heading);
  }
}

/**
 * Process paragraph breaks (.PP or .P macros)
 */
void process_paragraph_break(FILE *out) { fprintf(out, "\n"); }

/**
 * Process font formatting escape sequences like \fB (bold), \fI (italic), \fP
 * (previous) Returns a newly allocated string with markdown formatting
 */
char *process_font_escapes(const char *text) {
  size_t len = strlen(text);
  char *result =
      malloc(len * 2 + 1); // Allocate enough space for markdown formatting
  if (!result)
    return NULL;

  char *out = result;
  const char *in = text;

  // Keep track of the current formatting state
  enum FontState { NORMAL, BOLD, ITALIC } state = NORMAL;

  while (*in) {
    if (in[0] == '\\' && in[1] == 'f') {
      if (in[2] == 'B' || in[2] == 'b') {
        // Bold
        strcpy(out, "**");
        out += 2;
        in += 3;
        state = BOLD;
      } else if (in[2] == 'I' || in[2] == 'i') {
        // Italic
        strcpy(out, "*");
        out += 1;
        in += 3;
        state = ITALIC;
      } else if (in[2] == 'P' || in[2] == 'R' || in[2] == 'p' || in[2] == 'r') {
        // Previous or Roman (end of formatting)
        if (state == BOLD) {
          strcpy(out, "**");
          out += 2;
        } else if (state == ITALIC) {
          strcpy(out, "*");
          out += 1;
        }
        in += 3;
        state = NORMAL;
      } else {
        // Unknown font command, copy as is
        *out++ = *in++;
      }
    } else {
      *out++ = *in++;
    }
  }

  // Ensure we close any open formatting at the end
  if (state == BOLD) {
    strcpy(out, "**");
    out += 2;
  } else if (state == ITALIC) {
    strcpy(out, "*");
    out += 1;
  }

  *out = '\0';
  return result;
}

/**
 * Process indented paragraph (.IP macro)
 */
void process_indented_paragraph(const char *line, FILE *out) {
  char bullet[MAX_LINE_LEN];
  const char *start = line + 4; // Skip ".IP "
  char *formatted_bullet = NULL;

  // Extract the bullet text, handling quotes if present
  if (*start == '"') {
    start++;
    size_t len = strcspn(start, "\"");
    strncpy(bullet, start, len);
    bullet[len] = '\0';

    // Skip to next argument after the quoted text
    start += len + 1;
    while (*start && (*start == ' ' || *start == '\t'))
      start++;
  } else if (*start == '\\') {
    // This might be a font escape sequence like \fIname\fP
    strncpy(bullet, start, sizeof(bullet) - 1);
    bullet[sizeof(bullet) - 1] = '\0';

    // Remove newline and everything after the first space (width indicator)
    char *nl = strchr(bullet, '\n');
    if (nl)
      *nl = '\0';
    char *sp = strchr(bullet, ' ');
    if (sp) {
      *sp = '\0';
      // Skip to next argument
      start += (sp - bullet);
      while (*start && (*start == ' ' || *start == '\t'))
        start++;
    }

    // Process font escapes
    formatted_bullet = process_font_escapes(bullet);
  } else {
    size_t len = strcspn(start, " \t\n");
    strncpy(bullet, start, len);
    bullet[len] = '\0';

    // Skip to next argument
    start += len;
    while (*start && (*start == ' ' || *start == '\t'))
      start++;
  }

  // Check for size specification (like "1i")
  char width[32] = {0};
  if (*start && !isspace(*start)) {
    size_t len = strcspn(start, " \t\n");
    strncpy(width, start, len > 31 ? 31 : len);
  }

  // Use the processed bullet
  const char *bullet_to_use = formatted_bullet ? formatted_bullet : bullet;

  // Output as markdown list item or indented paragraph
  if (strcmp(bullet_to_use, "\\(bu") == 0 || strcmp(bullet_to_use, "*") == 0 ||
      strcmp(bullet_to_use, "•") == 0) {
    fprintf(out, "* ");
  } else if (isdigit(bullet_to_use[0])) {
    fprintf(out, "%s. ", bullet_to_use);
  } else {
    // For function arguments and similar items
    fprintf(out, "- %s ", bullet_to_use);
  }

  if (formatted_bullet)
    free(formatted_bullet);
}

/**
 * Process bolded text (.B macro)
 */
void process_bold_text(const char *line, FILE *out) {
  char text[MAX_LINE_LEN];
  const char *start = line + 3; // Skip ".B "

  // Extract the text to be bolded
  strncpy(text, start, sizeof(text) - 1);
  text[sizeof(text) - 1] = '\0';
  // Remove newline
  char *nl = strchr(text, '\n');
  if (nl)
    *nl = '\0';

  // Output as markdown bold
  fprintf(out, "**%s** ", text);
}

/**
 * Process italicized text (.I macro)
 */
void process_italic_text(const char *line, FILE *out) {
  char text[MAX_LINE_LEN];
  const char *start = line + 3; // Skip ".I "

  // Extract the text to be italicized
  strncpy(text, start, sizeof(text) - 1);
  text[sizeof(text) - 1] = '\0';
  // Remove newline
  char *nl = strchr(text, '\n');
  if (nl)
    *nl = '\0';

  // Output as markdown italic
  fprintf(out, "*%s*", text);
}

/**
 * Process SEE ALSO sections and create links to other functions
 */
void process_see_also(char *line, FILE *out) {
  char *p = line;
  char output_line[MAX_LINE_LEN * 2] = {
      0}; // Double the size to accommodate links
  char *o = output_line;

  // Skip leading whitespace
  while (*p && isspace(*p)) {
    *o++ = *p++;
  }

  while (*p) {
    // Look for function names (alphanumeric or underscore sequences)
    if (isalnum(*p) || *p == '_') {
      char function_name[MAX_LINE_LEN];
      char *fn = function_name;

      // Copy the function name
      while (*p && (isalnum(*p) || *p == '_')) {
        *fn++ = *p++;
      }
      *fn = '\0';

      // If it's followed by a comma, space, or end of line, it's likely a
      // function reference
      if (!*p || *p == ',' || isspace(*p)) {
        // Replace with a markdown link
        o += sprintf(o, "[%s](#%s)", function_name, function_name);
      } else {
        // Not a function name, just copy the original text
        strcpy(o, function_name);
        o += strlen(function_name);
      }
    } else {
      // Copy any other character as is
      *o++ = *p++;
    }
  }

  *o = '\0';

  // Write the processed line with links
  fprintf(out, "%s\n", output_line);
}

/**
 * Main function to convert man page to markdown
 */
void convert_man_to_md(FILE *in, FILE *out, bool merge_mode) {
  char line[MAX_LINE_LEN];
  char title[MAX_TITLE_LEN] = {0};
  int section = 0;
  bool is_in_code_block = false;
  bool is_in_see_also = false;
  bool is_in_references = false;
  char buffer[MAX_LINE_LEN * 2] = {0};
  size_t buffer_len = 0;

  // Get man page title and section
  if (get_man_title(in, title, &section)) {
    if (merge_mode) {
      fprintf(out, "## %s\n\n", title);
      // fprintf(out, "## %s(%d)\n\n", title, section);
      //  Create an anchor for linking to this function
      fprintf(out, "<a name=\"%s\"></a>\n\n", title);
    } else {
      fprintf(out, "# %s(%d)\n\n", title, section);
    }
  }

  // Reset file position to the beginning
  rewind(in);

  // Process each line
  while (fgets(line, sizeof(line), in) != NULL) {
    // Skip comments
    if (line[0] == '.' && line[1] == '\\' && line[2] == '"') {
      continue;
    }

    // Handle macros
    if (line[0] == '.') {
      // Flush any buffered content
      if ((is_in_see_also || is_in_references) && buffer_len > 0) {
        buffer[buffer_len] = '\0';
        if (is_in_see_also) {
          process_see_also(buffer, out);
        } else if (is_in_references) {
          // Process fonts in the references section
          char *processed = process_font_escapes(buffer);
          if (processed) {
            fprintf(out, "%s\n", processed);
            free(processed);
          } else {
            fprintf(out, "%s\n", buffer);
          }
        }
        buffer_len = 0;
        buffer[0] = '\0';
      }

      if (strncmp(line, ".TH ", 4) == 0) {
        // Title header - already processed
        continue;
      } else if (strncmp(line, ".SH ", 4) == 0) {
        // Section heading

        // Check if we're entering or leaving special sections
        bool was_in_special_section = is_in_see_also || is_in_references;
        is_in_see_also = (strstr(line, "SEE ALSO") != NULL);

        // If we were in a special section and are now leaving it, add extra
        // newline
        if (was_in_special_section && !(is_in_see_also || is_in_references)) {
          fprintf(out, "\n");
        }

        process_section_heading(line, out, merge_mode);
      } else if (strncmp(line, ".SS ", 4) == 0) {
        // Subsection heading
        process_subsection_heading(line, out, merge_mode);
      } else if (strncmp(line, ".PP", 3) == 0 || strncmp(line, ".P ", 3) == 0 ||
                 strncmp(line, ".LP", 3) == 0) {
        // Paragraph break (.PP, .P, or .LP)
        process_paragraph_break(out);
      } else if (strncmp(line, ".IP ", 4) == 0) {
        // Indented paragraph or list item
        process_indented_paragraph(line, out);
      } else if (strncmp(line, ".TP", 3) == 0) {
        // Term paragraph (used for definition lists)
        fprintf(out, "\n");
      } else if (strncmp(line, ".B ", 3) == 0) {
        // Bold text
        process_bold_text(line, out);
      } else if (strncmp(line, ".I ", 3) == 0) {
        // Italic text
        process_italic_text(line, out);
      } else if (strncmp(line, ".nf", 3) == 0) {
        // Start of no-fill region (code block)
        fprintf(out, "\n```c\n");
        is_in_code_block = true;
      } else if (strncmp(line, ".fi", 3) == 0) {
        // End of no-fill region
        fprintf(out, "```\n\n");
        is_in_code_block = false;
      } else {
        // Unhandled macro, output as comment
        fprintf(out, "<!-- %s -->\n", line);
      }
    } else {
      // Regular text line
      if ((is_in_see_also || is_in_references) && !is_in_code_block) {
        // For special sections, collect text in buffer for processing
        size_t line_len = strlen(line);
        if (buffer_len + line_len < sizeof(buffer) - 1) {
          strcat(buffer, line);
          buffer_len += line_len;
        }
      } else {
        // For regular text that might contain font escapes
        if (strstr(line, "\\f")) {
          // Line has font escape sequences, process it
          char *processed = process_font_escapes(line);
          if (processed) {
            fprintf(out, "%s", processed);
            free(processed);
          } else {
            fprintf(out, "%s", line);
          }
        } else {
          // Regular line with no font formatting
          fprintf(out, "%s", line);
        }
      }
    }
  }

  // Process any remaining buffered content
  if ((is_in_see_also || is_in_references) && buffer_len > 0) {
    buffer[buffer_len] = '\0';
    if (is_in_see_also) {
      process_see_also(buffer, out);
    } else if (is_in_references) {
      char *processed = process_font_escapes(buffer);
      if (processed) {
        fprintf(out, "%s\n", processed);
        free(processed);
      } else {
        fprintf(out, "%s\n", buffer);
      }
    }
  }

  // Ensure we close any open code block at the end of the file
  if (is_in_code_block) {
    fprintf(out, "```\n\n");
  }

  // Add a separator between man pages in merge mode
  if (merge_mode) {
    fprintf(out, "\n---\n\n");
  }
}
int main(int argc, char *argv[]) {
  Options opts = parse_options(argc, argv);
  FILE *out;

  // Open output file
  if (opts.output_file) {
    out = fopen(opts.output_file, "w");
    if (!out) {
      fprintf(stderr, "Error: Could not open output file '%s'\n",
              opts.output_file);
      return 1;
    }
  } else {
    out = stdout;
  }

  // Check if input is a directory
  struct stat path_stat;
  stat(opts.input_file, &path_stat);
  bool is_directory = S_ISDIR(path_stat.st_mode);

  if (is_directory) {
    // Process all files in directory in alphabetical order
    DIR *dir;
    struct dirent *entry;

    dir = opendir(opts.input_file);
    if (!dir) {
      fprintf(stderr, "Error: Could not open directory '%s'\n",
              opts.input_file);
      if (opts.output_file)
        fclose(out);
      return 1;
    }

    // First, collect all file names
    char **file_names = NULL;
    int file_count = 0;
    int file_capacity = 0;

    while ((entry = readdir(dir)) != NULL) {
      // Skip . and .. entries
      if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
        continue;
      }

      // Create full path to file
      char full_path[PATH_MAX];
      snprintf(full_path, PATH_MAX, "%s/%s", opts.input_file, entry->d_name);

      // Check if it's a regular file
      stat(full_path, &path_stat);
      if (!S_ISREG(path_stat.st_mode)) {
        continue;
      }

      // Add to our list of files
      if (file_count >= file_capacity) {
        file_capacity = file_capacity ? file_capacity * 2 : 16;
        file_names = realloc(file_names, file_capacity * sizeof(char *));
        if (!file_names) {
          fprintf(stderr, "Error: Memory allocation failed\n");
          closedir(dir);
          if (opts.output_file)
            fclose(out);
          return 1;
        }
      }

      file_names[file_count] = strdup(full_path);
      if (!file_names[file_count]) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        for (int i = 0; i < file_count; i++) {
          free(file_names[i]);
        }
        free(file_names);
        closedir(dir);
        if (opts.output_file)
          fclose(out);
        return 1;
      }

      file_count++;
    }

    closedir(dir);

    // Add a title for the merged document if we're in merge mode
    if (opts.merge_mode) {
      fprintf(out, "# Man Pages Documentation\n\n");
      fprintf(out, "This document contains merged man pages from `%s`.\n\n",
              opts.input_file);
    }

    // Sort the file names
    qsort(file_names, file_count, sizeof(char *), compare_strings);

    // Process each file in alphabetical order
    for (int i = 0; i < file_count; i++) {
      FILE *in = fopen(file_names[i], "r");
      if (!in) {
        fprintf(stderr, "Warning: Could not open file '%s', skipping\n",
                file_names[i]);
        continue;
      }

      if (opts.verbose) {
        printf("Processing %s\n", file_names[i]);
      }

      convert_man_to_md(in, out, opts.merge_mode);
      fclose(in);

      free(file_names[i]);
    }

    // Free the file names array
    free(file_names);
  } else {
    // Process a single file
    FILE *in = fopen(opts.input_file, "r");
    if (!in) {
      fprintf(stderr, "Error: Could not open input file '%s'\n",
              opts.input_file);
      if (opts.output_file)
        fclose(out);
      return 1;
    }

    if (opts.verbose) {
      printf("Converting %s\n", opts.input_file);
    }

    convert_man_to_md(in, out, opts.merge_mode);
    fclose(in);

    if (opts.verbose && opts.output_file) {
      printf("Output written to %s\n", opts.output_file);
    }
  }

  if (opts.output_file) {
    fclose(out);
  }

  return 0;
}

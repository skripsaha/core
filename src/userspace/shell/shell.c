// ============================================================================
// BOXOS SHELL - Innovative Tag-Based Command Interface
// ============================================================================
//
// Prompt: ~ (or [tag:value]~ with context)
// Syntax: ~ command arguments tags
//
// ============================================================================

#include "../ulib/ulib.h"

// ============================================================================
// SHELL STATE
// ============================================================================

#define MAX_CONTEXT_TAGS 8
#define MAX_ARGS 16

static Tag context_tags[MAX_CONTEXT_TAGS];
static int context_tag_count = 0;

static char* args[MAX_ARGS];
static int arg_count = 0;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Check if string contains ':'  (is a tag)
static int is_tag(const char* s) {
    while (*s) {
        if (*s == ':') return 1;
        s++;
    }
    return 0;
}

// Parse tag "key:value" into Tag structure
static int parse_tag(const char* s, Tag* tag) {
    int i = 0;

    // Copy key (before ':')
    while (s[i] && s[i] != ':' && i < TAG_KEY_SIZE - 1) {
        tag->key[i] = s[i];
        i++;
    }
    tag->key[i] = '\0';

    if (s[i] != ':') {
        // Special tag (no value) - just a word like "trashed", "system"
        tag->value[0] = '\0';
        return 1;
    }

    // Skip ':'
    i++;

    // Copy value (after ':')
    int j = 0;
    while (s[i] && j < TAG_VALUE_SIZE - 1) {
        tag->value[j++] = s[i++];
    }
    tag->value[j] = '\0';

    return 1;
}

// Print newline
static void newline(void) {
    print("\n");
}

// ============================================================================
// PROMPT
// ============================================================================

static void print_prompt(void) {
    // Print context tags if any
    if (context_tag_count > 0) {
        print_attr("[", VGA_PROMPT_TAG);
        for (int i = 0; i < context_tag_count; i++) {
            if (i > 0) print_attr(" ", VGA_PROMPT_TAG);
            print_attr(context_tags[i].key, VGA_PROMPT_TAG);
            if (context_tags[i].value[0]) {
                print_attr(":", VGA_PROMPT_TAG);
                print_attr(context_tags[i].value, VGA_PROMPT_TAG);
            }
        }
        print_attr("]", VGA_PROMPT_TAG);
    }

    // Print prompt symbol
    print_attr("~ ", VGA_PROMPT);
}

// ============================================================================
// COMMAND HANDLERS
// ============================================================================

// help - Show available commands
static void cmd_help(void) {
    print_attr("BoxOS Shell Commands:\n", VGA_SUCCESS);
    newline();

    print("  ");
    print_attr("help", VGA_LIGHT_CYAN);
    print("                    Show this help\n");

    print("  ");
    print_attr("clear", VGA_LIGHT_CYAN);
    print("                   Clear screen\n");

    print("  ");
    print_attr("me", VGA_LIGHT_CYAN);
    print("                      System information\n");

    print("  ");
    print_attr("say", VGA_LIGHT_CYAN);
    print(" <text>              Print text\n");

    print("  ");
    print_attr("use", VGA_LIGHT_CYAN);
    print(" <tags>              Set working context\n");

    print("  ");
    print_attr("use", VGA_LIGHT_CYAN);
    print("                      Clear context\n");

    newline();
    print_attr("File Commands:\n", VGA_SUCCESS);

    print("  ");
    print_attr("files", VGA_LIGHT_CYAN);
    print(" [tags]             List files (by tags)\n");

    print("  ");
    print_attr("create", VGA_LIGHT_CYAN);
    print(" <name> [tags]     Create file with tags\n");

    print("  ");
    print_attr("show", VGA_LIGHT_CYAN);
    print(" <name>              Show file contents\n");

    print("  ");
    print_attr("tag", VGA_LIGHT_CYAN);
    print(" <name> <tag>        Add tag to file\n");

    print("  ");
    print_attr("untag", VGA_LIGHT_CYAN);
    print(" <name> <key>      Remove tag from file\n");

    newline();
    print_attr("System Commands:\n", VGA_SUCCESS);

    print("  ");
    print_attr("reboot", VGA_LIGHT_CYAN);
    print("                  Reboot system\n");

    print("  ");
    print_attr("bye", VGA_LIGHT_CYAN);
    print("                     Shutdown\n");
}

// clear - Clear screen
static void cmd_clear(void) {
    clear();
}

// me - System information
static void cmd_me(void) {
    print_attr("BoxOS", VGA_SUCCESS);
    print(" - Kernel Workflow Engine\n");
    print("Architecture: x86-64\n");
    print("Shell: BoxOS Shell v1.0\n");

    if (context_tag_count > 0) {
        print("\nCurrent context: ");
        for (int i = 0; i < context_tag_count; i++) {
            if (i > 0) print(" ");
            print_attr(context_tags[i].key, VGA_PROMPT_TAG);
            if (context_tags[i].value[0]) {
                print_attr(":", VGA_PROMPT_TAG);
                print_attr(context_tags[i].value, VGA_PROMPT_TAG);
            }
        }
        newline();
    }
}

// say - Print text
static void cmd_say(void) {
    for (int i = 1; i < arg_count; i++) {
        if (i > 1) print(" ");
        print(args[i]);
    }
    newline();
}

// use - Set context tags
static void cmd_use(void) {
    // Clear old context
    context_tag_count = 0;

    // Parse new context tags from arguments
    for (int i = 1; i < arg_count && context_tag_count < MAX_CONTEXT_TAGS; i++) {
        if (is_tag(args[i]) || !contains_char(args[i], ' ')) {
            parse_tag(args[i], &context_tags[context_tag_count]);
            context_tag_count++;
        }
    }

    if (context_tag_count > 0) {
        print_attr("Context set to: ", VGA_SUCCESS);
        for (int i = 0; i < context_tag_count; i++) {
            if (i > 0) print(" ");
            print(context_tags[i].key);
            if (context_tags[i].value[0]) {
                print(":");
                print(context_tags[i].value);
            }
        }
        newline();
    } else {
        print_attr("Context cleared\n", VGA_HINT);
    }
}

// files - List files (stub for now)
static void cmd_files(void) {
    print_attr("TagFS File Listing:\n", VGA_SUCCESS);
    newline();

    // TODO: Actually query TagFS via events
    // For now, show a placeholder message
    print_attr("(TagFS query not yet implemented in user-space)\n", VGA_HINT);

    if (context_tag_count > 0) {
        print("Would filter by context: ");
        for (int i = 0; i < context_tag_count; i++) {
            if (i > 0) print(" ");
            print(context_tags[i].key);
            if (context_tags[i].value[0]) {
                print(":");
                print(context_tags[i].value);
            }
        }
        newline();
    }
}

// create - Create file (stub)
static void cmd_create(void) {
    if (arg_count < 2) {
        print_attr("Usage: create <name> [tags...]\n", VGA_ERROR);
        return;
    }

    print_attr("Would create file: ", VGA_SUCCESS);
    print(args[1]);
    newline();

    // Show tags that would be applied
    int tag_count = 0;
    for (int i = 2; i < arg_count; i++) {
        if (tag_count == 0) {
            print("With tags: ");
        }
        if (tag_count > 0) print(", ");
        print(args[i]);
        tag_count++;
    }
    if (tag_count > 0) newline();

    // Add context tags
    if (context_tag_count > 0) {
        print("Plus context tags: ");
        for (int i = 0; i < context_tag_count; i++) {
            if (i > 0) print(", ");
            print(context_tags[i].key);
            if (context_tags[i].value[0]) {
                print(":");
                print(context_tags[i].value);
            }
        }
        newline();
    }

    print_attr("(TagFS create not yet implemented in user-space)\n", VGA_HINT);
}

// show - Show file contents (stub)
static void cmd_show(void) {
    if (arg_count < 2) {
        print_attr("Usage: show <filename>\n", VGA_ERROR);
        return;
    }

    print_attr("Would show file: ", VGA_HINT);
    print(args[1]);
    newline();
    print_attr("(TagFS read not yet implemented in user-space)\n", VGA_HINT);
}

// tag - Add tag to file (stub)
static void cmd_tag(void) {
    if (arg_count < 3) {
        print_attr("Usage: tag <filename> <key:value>\n", VGA_ERROR);
        return;
    }

    print_attr("Would add tag ", VGA_HINT);
    print(args[2]);
    print_attr(" to file ", VGA_HINT);
    print(args[1]);
    newline();
    print_attr("(TagFS tag not yet implemented in user-space)\n", VGA_HINT);
}

// untag - Remove tag from file (stub)
static void cmd_untag(void) {
    if (arg_count < 3) {
        print_attr("Usage: untag <filename> <key>\n", VGA_ERROR);
        return;
    }

    print_attr("Would remove tag ", VGA_HINT);
    print(args[2]);
    print_attr(" from file ", VGA_HINT);
    print(args[1]);
    newline();
    print_attr("(TagFS untag not yet implemented in user-space)\n", VGA_HINT);
}

// reboot - Reboot system (stub)
static void cmd_reboot(void) {
    print_attr("Reboot not yet implemented (needs ACPI)\n", VGA_WARNING);
}

// bye - Shutdown (stub)
static void cmd_bye(void) {
    print_attr("Shutdown not yet implemented (needs ACPI)\n", VGA_WARNING);
    print_attr("Halting instead...\n", VGA_HINT);

    // Exit shell process
    exit(0);
}

// ============================================================================
// COMMAND PARSER
// ============================================================================

static void parse_and_execute(char* line) {
    // Reset args
    arg_count = 0;

    // Skip leading whitespace
    while (*line == ' ' || *line == '\t') line++;

    // Empty line
    if (!*line) return;

    // Tokenize
    char* token = strtok(line, " \t");
    while (token && arg_count < MAX_ARGS) {
        args[arg_count++] = token;
        token = strtok(NULL, " \t");
    }

    if (arg_count == 0) return;

    // Match command
    char* cmd = args[0];

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        cmd_help();
    }
    else if (strcmp(cmd, "clear") == 0) {
        cmd_clear();
    }
    else if (strcmp(cmd, "me") == 0) {
        cmd_me();
    }
    else if (strcmp(cmd, "say") == 0) {
        cmd_say();
    }
    else if (strcmp(cmd, "use") == 0) {
        cmd_use();
    }
    else if (strcmp(cmd, "files") == 0 || strcmp(cmd, "ls") == 0) {
        cmd_files();
    }
    else if (strcmp(cmd, "create") == 0) {
        cmd_create();
    }
    else if (strcmp(cmd, "show") == 0 || strcmp(cmd, "cat") == 0) {
        cmd_show();
    }
    else if (strcmp(cmd, "tag") == 0) {
        cmd_tag();
    }
    else if (strcmp(cmd, "untag") == 0) {
        cmd_untag();
    }
    else if (strcmp(cmd, "reboot") == 0) {
        cmd_reboot();
    }
    else if (strcmp(cmd, "bye") == 0 || strcmp(cmd, "exit") == 0 ||
             strcmp(cmd, "quit") == 0) {
        cmd_bye();
    }
    else {
        print_attr("Unknown command: ", VGA_ERROR);
        print(cmd);
        newline();
        print_attr("Type 'help' for available commands\n", VGA_HINT);
    }
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
    // Clear screen and show welcome
    clear();

    print_attr("=====================================\n", VGA_SUCCESS);
    print_attr("  BoxOS Shell - Welcome!\n", VGA_SUCCESS);
    print_attr("=====================================\n", VGA_SUCCESS);
    newline();
    print("Type ");
    print_attr("help", VGA_LIGHT_CYAN);
    print(" for available commands.\n");
    newline();

    // Main loop
    while (1) {
        print_prompt();

        char* line = readline();

        if (line && line[0]) {
            parse_and_execute(line);
        }
    }

    return 0;
}

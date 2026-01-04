#include <stdio.h>
#include <string.h>

int main(int, char**)
{
    char cmd_buffer[128];
    
    // Clear screen (optional, depends on terminal support)
    printf("\033[2J\033[H"); 

    printf("\n\n=== ToyOS Shell (User Space) ===\n");
    printf("Type 'help' for a list of commands.\n");

    while (true) {
        // Show prompt
        printf("ToyOS $> ");
        
        // Read input using our new LibC function
        fgets(cmd_buffer, 128, stdin);

        // Skip empty lines
        if (cmd_buffer[0] == '\0') continue;

        // Command parsing
        if (strcmp(cmd_buffer, "help") == 0) {
            printf("Available commands:\n");
            printf("  help    - Show this message\n");
            printf("  hello   - Test printf formatting\n");
            printf("  clear   - Clear the screen (simulated)\n");
        } 
        else if (strcmp(cmd_buffer, "hello") == 0) {
            printf("Hello! Testing numbers: decimal %d, hex %x\n", 123, 255);
        }
        else if (strcmp(cmd_buffer, "clear") == 0) {
             printf("\033[2J\033[H"); 
        }
        else {
            printf("Unknown command: %s\n", cmd_buffer);
        }
    }
}
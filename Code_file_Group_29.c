#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 256
#define MAX_COMMIT_MSG 100
#define MAX_PATH 256

typedef struct CommitNode {
    char commit_msg[MAX_COMMIT_MSG];
    time_t timestamp;
    struct CommitNode *next;
} CommitNode;

typedef struct LCTNode {
    char branch_name[50];
    struct LCTNode *parent;
    struct LCTNode *left;
    struct LCTNode *right;
    struct LCTNode *path_parent;
    char filename[50];
    CommitNode *working_node;
    struct LCTNode *next; // For linked list of branches
    int is_root; // To identify if it's the root of a link-cut tree
} LCTNode;

typedef struct VCS {
    LCTNode *current_branch;
    char directory[MAX_PATH];
    LCTNode *branches; // Head of the linked list of branches
} VCS;

VCS *vcs = NULL;

// Function prototypes
LCTNode* createBranch(LCTNode *parent, const char *branch_name);
LCTNode* findBranch(LCTNode* root, const char* branch_name);
void switchBranch(LCTNode* newBranch);
void resolveConflict(const char* line1, const char* line2, FILE* output);
void mergeFilesWithConflictResolution(const char* file1, const char* file2, const char* outputFile);
void mergeBranches(LCTNode* branch1, LCTNode* branch2, const char* outputFileName);
void create_commit(LCTNode *branch, const char *commit_msg);
void print_history(LCTNode* branch);
void display_menu();
void link(LCTNode* child, LCTNode* parent);
void cut(LCTNode* node);
LCTNode* findRoot(LCTNode* node);
void splay(LCTNode* node);
LCTNode* access(LCTNode* node);
void rotate(LCTNode *node);

// Additional LCT helper functions

void rotate(LCTNode *node) {
    LCTNode *parent = node->parent;
    if (!parent) return;

    LCTNode *grandparent = parent->parent;
    if (parent->left == node) {
        parent->left = node->right;
        if (node->right) node->right->parent = parent;
        node->right = parent;
    } else {
        parent->right = node->left;
        if (node->left) node->left->parent = parent;
        node->left = parent;
    }
    parent->parent = node;
    node->parent = grandparent;

    if (grandparent) {
        if (grandparent->left == parent) grandparent->left = node;
        else grandparent->right = node;
    }
}

void splay(LCTNode *node) {
    while (node->parent) {
        LCTNode *parent = node->parent;
        LCTNode *grandparent = parent->parent;

        if (grandparent) {
            if ((grandparent->left == parent) == (parent->left == node)) {
                rotate(parent);
            } else {
                rotate(node);
            }
        }
        rotate(node);
    }
}

LCTNode* access(LCTNode* node) {
    splay(node);
    if (node->right) {
        node->right->path_parent = node;
        node->right->parent = NULL;
        node->right = NULL;
    }
    LCTNode *last = node;

    while (node->path_parent) {
        LCTNode *parent = node->path_parent;
        splay(parent);
        if (parent->right) {
            parent->right->path_parent = parent;
            parent->right->parent = NULL;
        }
        parent->right = node;
        node->parent = parent;
        node->path_parent = NULL;
        last = parent;
    }
    splay(node);
    return last;
}

void link(LCTNode* child, LCTNode* parent) {
    access(child);
    access(parent);
    child->parent = parent;
    parent->right = child;
}

void cut(LCTNode* node) {
    access(node);
    if (node->left) {
        node->left->parent = NULL;
        node->left = NULL;
    }
}

LCTNode* findRoot(LCTNode* node) {
    access(node);
    while (node->left) node = node->left;
    access(node);
    return node;
}

// Add this function above createBranch function
void copyFileContents(const char *srcFile, const char *destFile) {
    FILE *src = fopen(srcFile, "r");
    FILE *dest = fopen(destFile, "w");

    if (!src) {
        printf("Error opening source file: %s\n", srcFile);
        return;
    }
    if (!dest) {
        printf("Error opening destination file: %s\n", destFile);
        fclose(src);
        return;
    }

    char buffer[MAX_LINE_LENGTH];
    while (fgets(buffer, sizeof(buffer), src) != NULL) {
        fputs(buffer, dest);
    }

    fclose(src);
    fclose(dest);
}


LCTNode* createBranch(LCTNode *parent, const char *branch_name) {
    LCTNode *newBranch = (LCTNode *)malloc(sizeof(LCTNode));
    if (!newBranch) {
        printf("Memory allocation failed for new branch.\n");
        return NULL;
    }
    
    strcpy(newBranch->branch_name, branch_name);
    newBranch->parent = NULL;
    newBranch->left = NULL;
    newBranch->right = NULL;
    newBranch->path_parent = NULL;
    newBranch->is_root = 1;
    newBranch->working_node = NULL;

    snprintf(newBranch->filename, sizeof(newBranch->filename), "%s.txt", branch_name);
    
    // Copy contents from the parent branch's file if it exists
    if (parent) {
        copyFileContents(parent->filename, newBranch->filename);
        link(newBranch, parent); // Link the new branch to the parent
    }

    // Add the new branch to the linked list of branches
    newBranch->next = vcs->branches; 
    vcs->branches = newBranch;

    printf("Branch '%s' created from '%s'.\n", branch_name, parent ? parent->branch_name : "initial branch");
    return newBranch;
}

// Finding a branch by name
LCTNode* findBranch(LCTNode* head, const char* branch_name) {
    LCTNode* current = head;
    while (current) {
        if (strcmp(current->branch_name, branch_name) == 0)
            return current; // Branch found
        current = current->next; // Move to the next branch
    }
    return NULL; // Branch not found
}

// Switching the current branch
void switchBranch(LCTNode* newBranch) {
    vcs->current_branch = newBranch;
    printf("Switched to branch '%s'.\n", newBranch->branch_name);
}

// Resolving conflicts during merging
void resolveConflict(const char* line1, const char* line2, FILE* output) {
    int choice;
    printf("Conflict detected:\n");
    printf("1. Keep version from branch 1: %s", line1);
    printf("2. Keep version from branch 2: %s", line2);
    printf("Enter your choice (1 or 2): ");
    scanf("%d", &choice);

    if (choice == 1) {
        fprintf(output, "%s", line1);
    } else {
        fprintf(output, "%s", line2);
    }
}


// Helper function to trim whitespace
void trimWhitespace(char *str) {
    char *end;

    // Trim leading space
    while (isspace((unsigned char)*str)) str++;

    // All spaces case
    if (*str == 0) return;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    // Write new null terminator
    *(end + 1) = '\0';
}

// Merging two files with conflict resolution
void mergeFilesWithConflictResolution(const char* file1, const char* file2, const char* outputFile) {
    FILE *fp1 = fopen(file1, "r");
    FILE *fp2 = fopen(file2, "r");
    FILE *output = fopen(outputFile, "w");

    if (!fp1 || !fp2 || !output) {
        perror("Error opening file for merging");
        return;
    }

    char line1[MAX_LINE_LENGTH];
    char line2[MAX_LINE_LENGTH];

    while (fgets(line1, MAX_LINE_LENGTH, fp1) && fgets(line2, MAX_LINE_LENGTH, fp2)) {
        trimWhitespace(line1);
        trimWhitespace(line2);

        if (strcmp(line1, line2) != 0) {
            resolveConflict(line1, line2, output);
        } else {
            fprintf(output, "%s\n", line1); // Writing without further trimming keeps original format
        }
    }

    while (fgets(line1, MAX_LINE_LENGTH, fp1)) {
        fprintf(output, "%s", line1);
    }

    while (fgets(line2, MAX_LINE_LENGTH, fp2)) {
        fprintf(output, "%s", line2);
    }

    fclose(fp1);
    fclose(fp2);
    fclose(output);
}
// Merging two branches into a new file
void mergeBranches(LCTNode* branch1, LCTNode* branch2, const char* outputFileName) {
    mergeFilesWithConflictResolution(branch1->filename, branch2->filename, outputFileName);
    printf("Branches '%s' and '%s' merged into '%s'.\n", branch1->branch_name, branch2->branch_name, outputFileName);
}

// Creating a commit in the current branch
void create_commit(LCTNode *branch, const char *commit_msg) {
    CommitNode *newCommit = (CommitNode *)malloc(sizeof(CommitNode));
    if (!newCommit) {
        printf("Memory allocation failed for commit.\n");
        return;
    }

    strncpy(newCommit->commit_msg, commit_msg, MAX_COMMIT_MSG);
    newCommit->timestamp = time(NULL);
    newCommit->next = branch->working_node; // Link to the previous commits
    branch->working_node = newCommit; // Update the working node

    printf("Commit '%s' added to branch '%s'.\n", commit_msg, branch->branch_name);
}

// Function to show history of all commits
void print_history(LCTNode* branch) {
    CommitNode *commit = branch->working_node;
    printf("Commit history for branch '%s':\n", branch->branch_name);

    while (commit) {
        char *timeStr = ctime(&commit->timestamp);
        timeStr[strcspn(timeStr, "\n")] = '\0'; // Remove newline character
        printf("[%s] %s\n", timeStr, commit->commit_msg);
        commit = commit->next; // Move to the next commit
    }
}

// Display the main menu
void display_menu() {
    printf("\nVersion Control System Menu:\n");
    printf("1. Create Commit\n");
    printf("2. Create Branch\n");
    printf("3. Switch Branch\n");
    printf("4. Merge Branches\n");
    printf("5. Show Commit History\n");
    printf("6. Exit\n");
    printf("Choose an option: ");
}

int main() {
    vcs = (VCS *)malloc(sizeof(VCS));
    if (!vcs) {
        printf("Memory allocation failed.\n");
        return 1;
    }

    // Initialize the VCS system with a default "master" branch
    vcs->branches = createBranch(NULL, "master");
    vcs->current_branch = vcs->branches;

    int choice;
    while (1) {
        display_menu();
        scanf("%d", &choice);

        switch (choice) {
            case 1: { // Create Commit
                char commit_msg[MAX_COMMIT_MSG];
                printf("Enter commit message: ");
                getchar();  // Clear newline from buffer
                fgets(commit_msg, sizeof(commit_msg), stdin);
                commit_msg[strcspn(commit_msg, "\n")] = 0;  // Remove newline
                create_commit(vcs->current_branch, commit_msg);
                break;
            }
            case 2: { // Create Branch
                char branch_name[50];
                printf("Enter branch name: ");
                scanf("%s", branch_name);
                LCTNode* newBranch = createBranch(vcs->current_branch, branch_name);
                if (newBranch) {
                    printf("Branch '%s' created successfully.\n", branch_name);
                } else {
                    printf("Failed to create branch '%s'.\n", branch_name);
                }
                break;
            }
            case 3: { // Switch Branch
                char branch_name[50];
                printf("Enter branch name to switch to: ");
                scanf("%s", branch_name);
                LCTNode *branch = findBranch(vcs->branches, branch_name);
                if (branch) {
                    switchBranch(branch);
                } else {
                    printf("Branch '%s' not found.\n", branch_name);
                }
                break;
            }
            case 4: { // Merge Branches
                char branch_name[50];
                printf("Enter branch name to merge with current branch: ");
                scanf("%s", branch_name);
                LCTNode *branch = findBranch(vcs->branches, branch_name);
                if (branch) {
                    char outputFileName[MAX_PATH];
                    snprintf(outputFileName, sizeof(outputFileName), "%s_%s_merged.txt", vcs->current_branch->branch_name, branch_name);
                    mergeBranches(vcs->current_branch, branch, outputFileName);
                } else {
                    printf("Branch '%s' not found.\n", branch_name);
                }
                break;
            }
            case 5: { // Show Commit History
                print_history(vcs->current_branch);
                break;
            }
            case 6: { // Exit
                printf("Exiting the Version Control System.\n");
                // Free allocated memory for branches and commits
                LCTNode* current = vcs->branches;
                while (current) {
                    LCTNode* to_free = current;
                    current = current->next;
                    // Free commits
                    CommitNode* commit = to_free->working_node;
                    while (commit) {
                        CommitNode* to_free_commit = commit;
                        commit = commit->next;
                        free(to_free_commit);
                    }
                    free(to_free);
                }
                free(vcs);
                return 0;
            }
            default:
                printf("Invalid option. Please try again.\n");
        }
    }

    return 0;
}
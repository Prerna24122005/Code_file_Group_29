# Code_file_Group_29

Building the Project
-------------------
1. Save the source code as 'vcs.c'

2. Compile the code using GCC:
   gcc -o vcs vcs.c

3. Run the executable:
   ./vcs

4. After compiling the program you have to create a master.txt file which will have the text content.After this a master branch will be automatically created.

Features and Usage
-----------------
Main Menu Options:
1. Create commit
2. Create branch
3. Switch branch
4. Merge branch
5. Show history
6. Exit

Basic Operations
---------------

Creating a New Branch:
- Select option 2 from the menu
- Enter the name for the new branch
- The system will create a new text file with the branch name (e.g., 'branchname.txt')
- Contents from the parent branch will be automatically copied to the new branch

Making Commits:
- Select option 1 from the menu
- Enter a commit message when prompted
- The commit will be saved with a timestamp in the current branch's history

Switching Branches:
- Select option 3 from the menu
- Enter the name of the branch you want to switch to
- The system will change the current working branch

Merging Branches:
- Select option 4 from the menu
- Enter the name of the branch you want to merge with the current branch
- When conflicts occur, you'll be prompted to choose between versions
- The merged result will be saved in a new file named 'branch1_branch2_merged.txt'

Viewing History:
- Select option 5 from the menu
- Displays all commits in the current branch with timestamps

File Structure
-------------
- Each branch is associated with a text file: '<branch_name>.txt'
- Merged branches create new files: '<branch1>_<branch2>_merged.txt'

Implementation Notes
------------------

Memory Management:
- The system dynamically allocates memory for new branches and commits
- Memory is freed when the program exits

Example Usage
------------
# Compile the program
gcc -o vcs vcs.c

# Run the program
./vcs

# Create a new branch
Select 2
Enter branch name: feature1

# Commit Changes
Select 1
Enter commit message: Initial commit

# Switch to another branch
Select 3
Enter branch name: feature1

# View history
Select 5

# Merge branches
Select 4
Enter branch name: master

#include <string>
#include <stack>
#include <sys/stat.h>
#include <dirent.h>
#include<unistd.h>
#define OPENDIR_ERROR 1
#define CLOSEDIR_ERROR 2
int print_info(const std::string &path, int lev)
{
    struct stat statbuf;
    lstat(path.c_str(), &statbuf);
    printf(("%" + std::to_string(lev * 6 + path.size()) + "s inode: %ld\n").c_str(), path.c_str(), statbuf.st_ino);
    return(0);
}

int walk(std::string root_path)
{
    struct dirent *dirp;
    DIR *dp;

    std::stack<std::string> stack;
    stack.push("");
    chdir(root_path.c_str());
    int lev = -1;

    while (!stack.empty())
    {
        chdir((std::string("./") + stack.top()).c_str());
        lev++;
        print_info(stack.top(), lev);
        stack.pop();

        if ((dp = opendir(".")) == NULL)
            return OPENDIR_ERROR;
        else {
            while ((dirp = readdir(dp)) != NULL) {
                if (strcmp(dirp->d_name, ".") != 0 &&
                    strcmp(dirp->d_name, "..") != 0)
                {

                    struct stat statbuf;
                    lstat(dirp->d_name, &statbuf);
                    if ((statbuf.st_mode & S_IFMT) != S_IFDIR)
                        print_info(dirp->d_name, lev + 1);
                    else {
                        stack.push("..");
                        stack.push(dirp->d_name);
                    }
                }
            }
            if (closedir(dp) != 0)
                return CLOSEDIR_ERROR;
        }
        while (!stack.empty() && stack.top() == "..") {
            chdir(stack.top().c_str());
            lev--;
            stack.pop();
        }

    }
    return 0;
}

int main() {
    walk("/Users/kate/Desktop");
    return 0;
}

# Git Terminal Interactive

![local-interaction-pannel]("https://github.com/poemdistance/Git-Terminal-Interactive/blob/add_picture/interaction_pannel.png" 'local-interaction-pannel')

### Dependencies

    ncurses library

### Compile

    gcc -g choice.c -o choice -lncurses -ltinfo -lmenu

### Run

    # copy this executable file to /usr/bin/xxx and rename, so that you can execute it everywhere
    ./choice 

### Example

    ./choice -r         # open remote branch interaction panel
    ./choice -l         # open local branch interaction panel
    ./choice -u         # update branch information from remote
    ./choice -rl test   # create 'test' branch in remote and local if 'test' is not exists, and then checkout to 'test'
    ./choice test       # create 'test' branch in local if 'test' is not exists (without any parameter, '-l' as default)
    ./chocide -dl test  # delete local branch 'test'

    ...

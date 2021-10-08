#include <unistd.h>
#include <menu.h>
#include <string.h>
#include <stdlib.h>
#include <curses.h>
#include <stdbool.h>
#include <stdarg.h>

#define ROW_NUM 20
#define COL_NUM 64
#define WIN_MENU_DIFF_ROW 6

#define DELETE_LOCAL_BRANCH_BIT     (1<<0)
#define DELETE_REMOTE_BRANCH_BIT    (1<<1)

#define LOCAL_BRANCH_INTERACTION    (1<<0)
#define REMOTE_BRANCH_INTERACTION   (1<<1)

#define HELP_MSG                    (1<<0)
#define DELETE_BRANCH_INTERACTION   (1<<1)
#define UPDATE_BRANCH_INFO          (1<<2)

#define ARGUMENT_FIRST        (1<<0)
#define OPTION_FRIST          (1<<1)

#define BRANCH_EXTRA_HINT_SIZE (1024)

#define TYPE_EXECUTE_FILE   (1<<0)
#define TYPE_PARAMETER      (1<<1)
#define TYPE_OPTION         (1<<2)

#define UNCONCERN_BRANCH  (0)
#define REMOTE_BRANCH     (1<<0)
#define LOCAL_BRANCH      (1<<1)

typedef struct BranchInfo {
    size_t interaction_object;
    size_t branch_count;
    char **branch_index;
    size_t *branch_location;
    char **branch_index_hint;
    size_t *branch_index_hint_extra_size;
    size_t current_branch_index;
    size_t *branch_operation_mark;
    bool drop_operation;
    struct BranchInfo *local_branch;
    struct BranchInfo *remote_branch;
} BranchInfo;

void print_in_middle(WINDOW *win, int y, int startx, int width, char *string, chtype color) {
    int x;

    if(win == NULL)
        win = stdscr;
    if(y == 0)
        getyx(win, y, x);
    x = startx + (width - strlen(string)) / 2;

    wattron(win, color);
    mvwprintw(win, y, x, "%s", string);
    wattroff(win, color);
    refresh();
}

void set_item_name(ITEM *item, const char* name)
{   
    item->name.str = name;
    item->name.length = strlen(name);
    item->description.str = name;
    item->description.length = strlen(name);
}

void repost_menu(MENU **menu, ITEM **items)
{
    unpost_menu(*menu);
    free_menu(*menu);
    *menu = new_menu(items);
}

void refresh_menu(WINDOW *win, MENU *menu, BranchInfo *branch_info)
{
    set_menu_win(menu, win);
    set_menu_sub(menu, derwin(win, ROW_NUM-WIN_MENU_DIFF_ROW, COL_NUM-5, 4, 2));
    set_menu_mark(menu, "-> ");
    menu_opts_off(menu, O_SHOWDESC);
    menu_opts_off(menu, O_ONEVALUE);
    set_menu_format(menu, ROW_NUM-WIN_MENU_DIFF_ROW, 1); // row, column

    box(win, 0, 0);
    if(branch_info->interaction_object & LOCAL_BRANCH_INTERACTION)
        print_in_middle(win, 1, 0, COL_NUM, "Git Branch Tools (Local)", COLOR_PAIR(0));
    else
        print_in_middle(win, 1, 0, COL_NUM, "Git Branch Tools (Remote)", COLOR_PAIR(0));

    mvwhline(win, 2, 1, ACS_HLINE, COL_NUM-2);
    mvprintw(LINES-16, 90, "o: checkout to selected branch");
    mvprintw(LINES-15, 90, "d: delete selected branch from local");
    mvprintw(LINES-14, 90, "r: remove selected branch from remote");
    mvprintw(LINES-13, 90, "k: key up");
    mvprintw(LINES-12, 90, "j: key down");
    mvprintw(LINES-11, 90, "a: drop/abort all operation and exit");
    mvprintw(LINES-10, 90, "g: jump to fist branch");
    mvprintw(LINES-9,  90, "G: jump to last branch");
    mvprintw(LINES-8,  90, "q/enter: exit and commit all operation");

    refresh();

    post_menu(menu);
    wrefresh(win);
}

void bit_reverse(size_t *target, size_t bit)
{
    *target = *target ^ bit;
}

size_t get_and_reset_bit(size_t *src, size_t bit)
{
    size_t is_set = *src & bit;
    *src = *src & (~bit);
    return is_set;
}

size_t get_bit(size_t src, size_t bit)
{
    return (src & bit);
}

void concat_extra_msg(
        char **src,
        char *extra_msg,
        size_t *origin_extra_size,
        size_t new_extra_size)
{
    if(new_extra_size > *origin_extra_size)
    {
        size_t new_size = (*origin_extra_size * 2) + new_extra_size;
        *src = realloc(*src, new_size);
        *origin_extra_size = new_size;
    }

    strcat(*src, extra_msg);
}

void set_branch_hint(MENU *menu,
        char *branch,
        char *branch_hint,
        size_t *branch_hint_extra_size,
        size_t operation_mark,
        size_t branch_location)
{
    strcpy(branch_hint, branch);
    size_t new_extra_hint_size = 0;
    char *del_local_hint = " [del-local]";
    char *del_remote_hint = " [del-remote]";
    char *remote_hint = " [remote]";

    strcpy(branch_hint, branch);

    if(branch_location & REMOTE_BRANCH)
    {
        new_extra_hint_size += strlen(remote_hint);

        concat_extra_msg(
                &branch_hint, remote_hint,
                branch_hint_extra_size,
                new_extra_hint_size);
    }

    if(operation_mark & DELETE_LOCAL_BRANCH_BIT)
    {
        new_extra_hint_size += strlen(del_local_hint);

        concat_extra_msg(&branch_hint,
                del_local_hint,
                branch_hint_extra_size,
                new_extra_hint_size);
    }

    if(operation_mark & DELETE_REMOTE_BRANCH_BIT)
    {
        new_extra_hint_size += strlen(del_remote_hint);

        concat_extra_msg(&branch_hint,
                del_remote_hint,
                branch_hint_extra_size,
                new_extra_hint_size);
    }

    set_item_name(current_item(menu), branch_hint);
}

ITEM *new_item_with_hint(BranchInfo *branch_info, size_t index)
{
    size_t new_extra_hint_size = 0;
    char *remote_hint = " [remote]";

    strcpy(branch_info->branch_index_hint[index], branch_info->branch_index[index]);

    if(branch_info->branch_location && branch_info->branch_location[index] & REMOTE_BRANCH)
    {
        new_extra_hint_size += strlen(remote_hint);

        concat_extra_msg(
                &(branch_info->branch_index_hint[index]),
                remote_hint,
                &(branch_info->branch_index_hint_extra_size[index]),
                new_extra_hint_size);
    }

    return new_item(
            branch_info->branch_index_hint[index],
            branch_info->branch_index_hint[index]);
}

char *choice_interactive( BranchInfo *branch_info)
{
    MENU *menu;
    WINDOW *win;
    ITEM **items = NULL, *tmp_item = NULL;

    int c;
    size_t choice = 0;
    bool checkout_branch = false;
    bool need_to_refresh_menu = false;

    initscr();
    start_color();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    init_pair(1, COLOR_RED, COLOR_BLACK);

    items = calloc(branch_info->branch_count+1, sizeof(ITEM *));
    for(size_t i=0; i<branch_info->branch_count; i++)
        items[i] = new_item_with_hint(branch_info, i);

    menu = new_menu((ITEM **)items);

    /* newwin(int nlines, int ncols, int begin_y, int begin_x)*/
    win = newwin(ROW_NUM, COL_NUM, 5, 12);
    keypad(win, TRUE);
    refresh_menu(win, menu, branch_info);
    set_current_item(menu, items[branch_info->current_branch_index]);

    while((c = wgetch(win)) != KEY_F(1))
    { 
        choice = item_index(current_item(menu));

        switch(c)
        {
            case 'j': case 'J': case KEY_DOWN:
                if(item_index(current_item(menu)) == branch_info->branch_count-1)
                {
                    set_current_item(menu, items[0]);
                    break;
                }
                menu_driver(menu, REQ_DOWN_ITEM);
                break;
            case 'k': case 'K': case KEY_UP:
                if(item_index(current_item(menu)) == 0)
                {
                    set_current_item(menu, items[branch_info->branch_count-1]);
                    break;
                }
                menu_driver(menu, REQ_UP_ITEM);
                break;
            case 'G':
                menu_driver(menu, REQ_LAST_ITEM);
                break;
            case 'g':
                menu_driver(menu, REQ_FIRST_ITEM);
                break;
            case 'm':
                menu_driver(menu, REQ_SCR_DPAGE);
                break;
            case 'n':
                menu_driver(menu, REQ_SCR_UPAGE);
                break;
            case ' ':
                menu_driver(menu, REQ_TOGGLE_ITEM);
                break;
            case 'o': case 'O':
                tmp_item = current_item(menu);
                choice = item_index(tmp_item);
                move(LINES - 3, 0);
                clrtoeol();
                mvprintw(LINES - 3, 0, "%s", item_name(tmp_item));
                refresh();
                checkout_branch = true;
                goto exit;

            case 10:
                goto exit;

            case 'a': /*case 27: // ESC */

                branch_info->drop_operation = true;
                goto exit;

            case 'd': case 'D':
                need_to_refresh_menu = true;
                bit_reverse(
                        &(branch_info->branch_operation_mark[choice]),
                        DELETE_LOCAL_BRANCH_BIT);
                break;
            case 'r': case 'R':
                need_to_refresh_menu = true;
                bit_reverse(
                        &(branch_info->branch_operation_mark[choice]),
                        DELETE_REMOTE_BRANCH_BIT);
                break;

            case 'q':case 'Q': case 67:
                goto exit;
        }

        if(need_to_refresh_menu)
        {
            need_to_refresh_menu = false;

            set_branch_hint(menu,
                    branch_info->branch_index[choice],
                    branch_info->branch_index_hint[choice],
                    &(branch_info->branch_index_hint_extra_size[choice]),
                    branch_info->branch_operation_mark[choice],
                    branch_info->branch_location[choice]);

            repost_menu(&menu, items);
            refresh_menu(win, menu, branch_info);
            set_current_item(menu, items[choice]);
        }

        wrefresh(win);
    }

exit:
    unpost_menu(menu);
    free_menu(menu);
    for(size_t i=0; i<branch_info->branch_count; i++)
        free_item(items[i]);
    free(items);
    endwin();

    if(branch_info->drop_operation)
        return NULL;

    return checkout_branch ? branch_info->branch_index[choice] : NULL;
}

bool is_space(int arg)
{
    return \
        arg == ' '  || \
        arg == '\t' || \
        arg == '\r' || \
        arg == '\v' || \
        arg == '\f' || \
        arg == '\r';
}

bool is_newline(int arg)
{
    return arg == '\n';
}

int get_raw_output_from_git_branch(char *git_command, char **input_buf)
{
    FILE *fp = popen(git_command, "r");
    if(fp == NULL)
    {
        fprintf(stderr, "popen(\"git branch\") execute failed. line: %d\n", __LINE__);
        exit(1);
    }

#define BASE_READ_SIZE 1024

    char read_buf[BASE_READ_SIZE] = { '\0' };
    size_t read_size = 0;

    size_t current_raw_size = BASE_READ_SIZE;
    char *raw_buf = calloc(current_raw_size, sizeof(char));
    size_t offset = 0;

    /* update the address pointed to by input_buf*/
    *input_buf = raw_buf;

    /* Read all text from pipleline, and dynamic allocate memory to hold the output text.*/
    while(true)
    {
        /* NOTE: reach the trailing character: '\0' will return read_size = 0*/
        if( (read_size = fread(read_buf, sizeof(char), sizeof(read_buf), fp)) < 0 )
        {
            fprintf(stderr, "fread data from popen file pointer failed. line: %d\n", __LINE__);
            exit(1);
        }

        /* printf("read_size: %ld sizeof(read_buf): %ld read_buf: %s<\n", */
        /*         read_size, sizeof(read_buf), read_buf); */



        /* Read the ending character: '\0';*/
        if(read_size == 0)
        {
            /* printf("last time reach the trailing character: '\0'.\n"); */
            raw_buf[offset] = '\0';
            break;
        }

        /* No more text found from pipeline*/
        if(read_size < sizeof(read_buf))
        {
            /* trailing character will not be read into read_buf, we need to add it ourselves*/
            read_buf[read_size] = '\0';

            strcpy(raw_buf+offset, read_buf);
            /* printf("No more text found from pipeline, %d %d\n",\ */
            /*     read_size < sizeof(read_buf), read_buf[sizeof(read_buf)-1] == '\0'); */
            break;
        }

        /* raw_buff is no more space to hold the pipeline text, alloc large memory*/
        if(offset+read_size >= current_raw_size)
        {
            current_raw_size *= 2;
            *input_buf = raw_buf = realloc(raw_buf, current_raw_size);
            /* printf("raw_buff: no more space to hold the pipeline text, alloc large memory" */
            /*         ", realloc size: %zu\n", current_raw_size); */
        }

        strcpy(raw_buf+offset, read_buf);
        offset += read_size;
    }

    /* printf("git branch output:\n<%s>\n", raw_buf); */

    return 0;
}

int parse_raw_output_of_git_branch_r( char *raw_buf, BranchInfo *branch_info)
{
    size_t base_branch_size = 6;
    size_t branch_count = 0;
    size_t max_branch_size = base_branch_size;
    char **branch_index = calloc(base_branch_size, sizeof(char*));
    char **dup_branch_index = calloc(base_branch_size, sizeof(char*));
    size_t *dup_branch_index_extra_size = calloc(base_branch_size, sizeof(size_t));

    branch_info->branch_location = calloc(base_branch_size, sizeof(size_t));

    /* update the address pointed to by input_buf*/
    branch_info->branch_index = branch_index;
    branch_info->branch_index_hint = dup_branch_index;
    branch_info->branch_index_hint_extra_size = dup_branch_index_extra_size;

    char *char_ptr = raw_buf;
    char *branch_name_start = NULL;
    bool branch_not_found = true;
    bool had_skipped_first_branch = false;
    while( *char_ptr )
    {
        if(is_space(*char_ptr))
        {
            char_ptr++;
            continue;
        }
        else if(branch_not_found)
        {
            /* substract the lenght of charater '\0'*/
            branch_name_start = char_ptr + sizeof("origin/") - 1; 
            branch_not_found = false;
        }

        if(is_newline(*char_ptr) && !had_skipped_first_branch )
        {
            char_ptr++;
            had_skipped_first_branch = true;
            branch_not_found = true;
            continue;
        }

        /* NOTE: did not handle the not newline trailing text*/
        if(had_skipped_first_branch && is_newline(*char_ptr))
        {
            *char_ptr = '\0';
            branch_index[branch_count] = calloc(sizeof(char), strlen(branch_name_start)+1);

            dup_branch_index[branch_count] = 
                calloc(sizeof(char), strlen(branch_name_start) + 1 + BRANCH_EXTRA_HINT_SIZE);

            dup_branch_index_extra_size[branch_count] = BRANCH_EXTRA_HINT_SIZE;
            strcpy(branch_index[branch_count], branch_name_start);

            branch_info->branch_location[branch_count] |= UNCONCERN_BRANCH;

            branch_count++;

            /* printf("found branch: %s\n", branch_index[branch_count-1]); */

            branch_not_found = true;

            if(branch_count == max_branch_size)
            {
                max_branch_size *= 2;
                branch_info->branch_index = branch_index =
                    realloc(branch_index, max_branch_size*sizeof(char*));

                branch_info->branch_index_hint = dup_branch_index =
                    realloc(dup_branch_index, max_branch_size*sizeof(char*));

                branch_info->branch_index_hint_extra_size = dup_branch_index_extra_size =
                    realloc(dup_branch_index_extra_size, max_branch_size*sizeof(size_t));

                branch_info->branch_location =
                    realloc(branch_info->branch_location, max_branch_size);

                /* printf("branch count > max branch size, realloc branch size: %ld\n", */
                /*         max_branch_size); */
            }
        }

        char_ptr++;
    }

    /* printf("branch count: %ld \n", branch_count); */

    return branch_count;
}

bool is_remote_branch(char *branch_name)
{
    return strstr(branch_name, "remotes/origin") == branch_name;
}

char *exclude_remote_prefix(char *branch_name)
{
    char *start = NULL;
    start = branch_name + strlen("remotes/origin/");

    if(strstr(start, "HEAD") == start)
        return start + strlen("HEAD -> origin/");

    return start;
}

char *get_real_branch_name(char *str)
{
    if(str[0] != '*')
        return str;

    /* pointer to the position after character '*' */
    char *char_ptr = str + 1;
    char *branch_name_start = char_ptr;
    while(*char_ptr)
    {
        if(is_space(*char_ptr) )
        {
            char_ptr++;
            continue;
        }

        branch_name_start = char_ptr;
        break;
    }

    return branch_name_start;
}

bool branch_is_stored(BranchInfo *branch_info, char *checking_branch)
{
    for(size_t i=0; i<branch_info->branch_count; i++)
    {
        if(strcmp(get_real_branch_name(branch_info->branch_index[i]), checking_branch) == 0)
            return true;
    }

    return false;
}

int parse_raw_output_of_git_branch( char *raw_buf, BranchInfo *branch_info)
{
    size_t base_branch_size = 6;
    size_t max_branch_size = base_branch_size;
    char **branch_index = calloc(base_branch_size, sizeof(char*));
    char **dup_branch_index = calloc(base_branch_size, sizeof(char*));
    size_t *dup_branch_index_extra_size = calloc(base_branch_size, sizeof(size_t));

    branch_info->branch_location = calloc(base_branch_size, sizeof(size_t));

    branch_info->branch_index = branch_index;
    branch_info->branch_index_hint = dup_branch_index;
    branch_info->branch_index_hint_extra_size = dup_branch_index_extra_size;

    char *char_ptr = raw_buf;
    char *branch_name_start = NULL;
    bool branch_not_found = true;
    while( *char_ptr )
    {
        if(is_space(*char_ptr))
        {
            char_ptr++;
            continue;
        }
        else if(branch_not_found)
        {
            branch_name_start = char_ptr;
            branch_not_found = false;
            if(*branch_name_start == '*')
                branch_info->current_branch_index = branch_info->branch_count;
        }

        /* NOTE: did not handle the not newline trailing text*/
        if(is_newline(*char_ptr))
        {
            *char_ptr = '\0';
            branch_not_found = true;

            if(is_remote_branch(branch_name_start))
            {
                /* is seems that no need to show the remote branch in local branch 
                 * interaction pannel. so disable showing remote branch in
                 * local branch interaction pannel in current time.*/
                char_ptr++;
                continue;

                branch_name_start = exclude_remote_prefix(branch_name_start);
                branch_info->branch_location[branch_info->branch_count] |= REMOTE_BRANCH;
            }

            if(branch_is_stored(branch_info, branch_name_start))
            {
                char_ptr++;
                continue;
            }

            size_t branch_name_size = strlen(branch_name_start) + 1;
            branch_index[branch_info->branch_count] 
                = calloc(sizeof(char), branch_name_size);

            dup_branch_index[branch_info->branch_count] = 
                calloc(sizeof(char), strlen(branch_name_start) + 1 + BRANCH_EXTRA_HINT_SIZE);

            dup_branch_index_extra_size[branch_info->branch_count] = BRANCH_EXTRA_HINT_SIZE;
            strcpy(branch_index[branch_info->branch_count], branch_name_start);

            branch_info->branch_count++;

            /* printf("found branch: %s\n", branch_index[branch_info->branch_count-1]); */

            if(branch_info->branch_count == max_branch_size)
            {
                max_branch_size *= 2;
                branch_info->branch_index = branch_index = 
                    realloc(branch_index, max_branch_size*sizeof(char*));

                branch_info->branch_index_hint = dup_branch_index =
                    realloc(dup_branch_index, max_branch_size*sizeof(char*));

                branch_info->branch_index_hint_extra_size = dup_branch_index_extra_size =
                    realloc(dup_branch_index_extra_size, max_branch_size*sizeof(size_t));

                branch_info->branch_location =
                    realloc(branch_info->branch_location, max_branch_size);

                /* printf("branch count > max branch size, realloc branch size: %ld\n", */
                /*         max_branch_size); */
            }
        }

        char_ptr++;
    }

    /* printf("branch count: %ld \n", branch_info->branch_count); */

    return branch_info->branch_count;
}


int get_all_branch_name(
        char *get_branch_command,
        int(*parse_raw_output)(char*, BranchInfo*),
        BranchInfo *branch_info)
{
    char *raw_buf = NULL;
    get_raw_output_from_git_branch(get_branch_command, &raw_buf);

    branch_info->branch_count = parse_raw_output(raw_buf, branch_info);

    branch_info->branch_operation_mark 
        = calloc(branch_info->branch_count, sizeof(size_t));

    free(raw_buf);

    return 0;
}


void command_execute(char *parameter,...)
{
    if(!parameter)
        return;

    va_list valist;
    va_start(valist, parameter);
    char *str = NULL;
    size_t command_length = strlen(parameter) + 1;

    while((str = va_arg(valist, char*)) != NULL)
        command_length += strlen(str);

    char *command = calloc(sizeof(char), command_length);
    strcat(command, parameter);

    va_start(valist, parameter);
    while((str = va_arg(valist, char*)) != NULL)
        strcat(command, str);

    va_end(valist);

    printf("execute command: %s\n", command);

    system(command);
    free(command);
}

void update_remote_references()
{
    command_execute("git fetch --all --prune", NULL);
}

void interactive_delete_branch( char **branch_index, size_t branch_count, size_t *branch_operation_mark)
{
    char *interactive_delete_branch_name = NULL;
    char *git_command = NULL;
    char *delete_remote_branch_command = "git push origin --delete ";
    char *delete_local_branch_command = "git branch -D ";
    for(size_t i=0; i<branch_count; i++)
    {
        if(!branch_operation_mark[i])
            continue;

        for(short bit=0; bit<64; bit++)
        {
            /* no operation mark, break to check next branch*/
            if(!branch_operation_mark[i])
                break;

            switch(get_and_reset_bit(&(branch_operation_mark[i]), 1 << bit))
            {
                case DELETE_LOCAL_BRANCH_BIT:
                    git_command = delete_local_branch_command;
                    break;
                case DELETE_REMOTE_BRANCH_BIT:
                    git_command = delete_remote_branch_command;
                    break;
                default:
                    continue;
            }

            interactive_delete_branch_name = get_real_branch_name(branch_index[i]);

            if(git_command == delete_local_branch_command && branch_index[i][0] == '*')
            {
                fprintf(stderr, "delete current local branch is forbbiden, skipping...\n");
                continue;
            }

            command_execute(git_command, interactive_delete_branch_name, NULL);
        }
    }
}

bool create_branch_if_not_exists(BranchInfo *branch_info, char *branch_name)
{
    char *real_branch_name = NULL;
    for(size_t i=0; i<branch_info->branch_count; i++)
    {
        real_branch_name = get_real_branch_name(branch_info->branch_index[i]);
        if(strcmp(branch_name, real_branch_name) == 0)
            return false;
    }

    printf("not found branch: [%s] creating...\n", branch_name);

    command_execute("git checkout -b ", branch_name, NULL);

    return true;
}

void switch_branch(BranchInfo *branch_info, char *choice_branch_name)
{
    if(!choice_branch_name)
        return;

    char *real_branch_name = get_real_branch_name(choice_branch_name);
    if(create_branch_if_not_exists(branch_info, real_branch_name))
        return;

    printf("switch_branch: %s\n", choice_branch_name);

    command_execute("git checkout ", real_branch_name, NULL);
}

void remote_branch_interation(BranchInfo *remote_branch)
{
    remote_branch->interaction_object = REMOTE_BRANCH_INTERACTION;
    char *choice_branch_name = choice_interactive(remote_branch);

    if(remote_branch->drop_operation)
        goto exit;
    else
        goto commit_operation;

commit_operation:

    interactive_delete_branch(
            remote_branch->branch_index,
            remote_branch->branch_count,
            remote_branch->branch_operation_mark);

    switch_branch(remote_branch->local_branch, choice_branch_name);

exit:
    return;
}

void local_branch_interaction(BranchInfo *local_branch)
{
    local_branch->drop_operation = false;
    local_branch->interaction_object = LOCAL_BRANCH_INTERACTION;

    char *choice_branch_name = choice_interactive(local_branch);

    if(local_branch->drop_operation)
        goto exit;
    else
        goto commit_operation;


commit_operation:

    printf("commit operation.\n");

    switch_branch(local_branch, choice_branch_name);

    interactive_delete_branch(
            local_branch->branch_index,
            local_branch->branch_count,
            local_branch->branch_operation_mark);

exit:
    return;
}

void append_parameter(char ***src, char *str)
{
    /* locate unused memory address and the lastest memory block index*/
    size_t free_index = UINT64_MAX;
    size_t last_index = UINT64_MAX;
    for(size_t i=0; ; i++)
    {
        if(free_index == UINT64_MAX && (*src)[i] == NULL)
        {
            free_index = i;
            break; /* found the free memory block just break and then copy str into it*/
        }

        if((*src)[i] == (void*)UINT64_MAX)
        {
            last_index = i;
            continue;
        }
    }

    /* allocate more memory while not found free memory block */
    if(free_index == UINT64_MAX)
    {
        size_t new_size = (last_index+1)*2;
        printf("not more space to store the parameter: %s realloc: %ld\n", str, new_size);
        (*src)[last_index] = NULL;
        (*src)[new_size-1] = (void*)UINT64_MAX; /* mark the end of the memory block*/
        free_index = last_index;
    }

    (*src)[free_index] = calloc(strlen(str)+1, sizeof(char));
    strcpy((*src)[free_index], str);
}

size_t parse_input_parameters(
        int argc,
        char **argv,
        size_t **object_set,
        size_t **feature_set,
        char ****manipulate_target)
{
    char *char_ptr = NULL;
    size_t set_index = 0;
    int format = 0;

    if(argc <= 1)
        return 0;

    format = ARGUMENT_FIRST;
    if(argv[1][0] != '-')
        format = OPTION_FRIST;

    /* calculate options count*/
    size_t option_count = 0;
    int previous_type = TYPE_EXECUTE_FILE;
    int current_type = previous_type;
    for(size_t i=1; i<argc; i++)
    {
        if(argv[i][0] == '-')
            current_type = TYPE_OPTION;
        else
            current_type = TYPE_PARAMETER;

        if(current_type != previous_type)
        {
            previous_type = current_type;
            option_count++;
        }
    }

    if(option_count > 1)
        option_count--;

#define PARAMETER_BASE_COUNT 2

    *object_set = calloc(option_count, sizeof(size_t));
    *feature_set = calloc(option_count, sizeof(size_t));
    *manipulate_target = calloc(option_count, sizeof(char**));
    for(size_t i=0; i<option_count; i++)
    {
        (*manipulate_target)[i] = calloc(PARAMETER_BASE_COUNT, sizeof(char*));
        (*manipulate_target)[i][PARAMETER_BASE_COUNT-1] = (void*)UINT64_MAX;
    }

    for(int i=1; i<argc; i++)
    {
        char_ptr = argv[i];

        if(i>=3 && format==OPTION_FRIST && *char_ptr!='-' && argv[i-1][0] == '-')
            set_index++;
        if(i>=3 && format==ARGUMENT_FIRST && *char_ptr=='-' && argv[i-1][0] != '-')
            set_index++;

        /* extract parameter*/
        if(*char_ptr != '-')
        {
            append_parameter(&(*manipulate_target)[set_index], char_ptr);
            continue;
        }

        char_ptr++; /* pointe to the character after '-'*/

        /* extract option*/
        while(*char_ptr)
        {
            switch(*char_ptr)
            {
                case 'r':
                    (*object_set)[set_index] |= REMOTE_BRANCH_INTERACTION;    break;
                case 'l':
                    (*object_set)[set_index] |= LOCAL_BRANCH_INTERACTION;     break;
                case 'd':
                    (*feature_set)[set_index] |= DELETE_BRANCH_INTERACTION;   break;
                case 'u':
                    (*feature_set)[set_index] |= UPDATE_BRANCH_INFO;          break;
                case 'h':
                    (*feature_set)[set_index] |= HELP_MSG;                    break;

                default:
                    fprintf(stderr, "unknown parameter: %c\n", *char_ptr);
                    break;
            }
            char_ptr++;
        }
    }

    return option_count;
}

bool is_parameter_legal(size_t object_set, size_t feature_set)
{
    return true;
}

void print_help_msg()
{
    printf("Usage: \n");
    printf("    -r manipulate remote branch\n");
    printf("    -l manipulate local branch\n");
    printf("    -d delete branch\n");
    printf("    -u update branch info\n");
    printf("    -h show this help message\n");
}

char *get_last_input_branch(char **manipulate_target)
{
    if(manipulate_target == NULL)
        return NULL;

    size_t i = 0;
    for(;;i++)
    {
        if(manipulate_target[i] == (void*)UINT64_MAX)
            break;

        if(manipulate_target[i] == NULL)
            break;
    }

    if(i == 0)
        return NULL;

    return manipulate_target[i-1];
}

void remote_newline(char *str)
{
    if(!str)
        return;

    char *char_ptr = str;
    while(*char_ptr)
    {
        if(*char_ptr == '\n')
            *char_ptr = '\0';

        char_ptr++;
    }
    return;
}

void create_remote_branch(char *branch_name)
{
    char *raw_buf = NULL;
    get_raw_output_from_git_branch("git show -s --format=%H", &raw_buf);

    remote_newline(raw_buf);

    command_execute("git push origin ", raw_buf, ":refs/heads/", branch_name, NULL);

    if(raw_buf)
        free(raw_buf);
}

void command_line_delete_branch(size_t object_set, char **manipulate_target)
{
    for(size_t i=0; manipulate_target[i] != (void*)UINT64_MAX; i++)
    {
        if(manipulate_target[i] == NULL)
            break;

        if(object_set & REMOTE_BRANCH_INTERACTION)
            command_execute("git push origin --delete ", manipulate_target[i], NULL);

        if(object_set & LOCAL_BRANCH_INTERACTION)
            command_execute("git branch -D ", manipulate_target[i], NULL);
    }
}

void command_line_update_branch_info(size_t object_set)
{
    if(object_set & REMOTE_BRANCH_INTERACTION)
        update_remote_references();

    if(object_set & LOCAL_BRANCH_INTERACTION)
        command_execute("git remote update origin --prune", NULL);
}

void run_interaction(size_t object_set, size_t feature_set, char **manipulate_target)
{
    if(!is_parameter_legal(object_set, feature_set))
        return;

    BranchInfo local_branch  = { '\0' };
    BranchInfo remote_branch = { '\0' };
    BranchInfo *branch_infos[2] = { &local_branch, &remote_branch };

    get_all_branch_name( "git branch -a", parse_raw_output_of_git_branch, &local_branch);
    get_all_branch_name( "git branch -r", parse_raw_output_of_git_branch_r, &remote_branch);

    local_branch.local_branch = &local_branch;
    local_branch.remote_branch = &remote_branch;
    remote_branch.local_branch = &local_branch;
    remote_branch.remote_branch = &remote_branch;


    /* default manipulate local branch*/
    if(!object_set)
        object_set |= LOCAL_BRANCH_INTERACTION;

    if(!feature_set)
    {
        char *last_input_branch = get_last_input_branch(manipulate_target);
        for(size_t i=0; i<64; i++)
        {
            if(!object_set)
                break;

            switch(get_and_reset_bit(&object_set, 1<<i))
            {
                case 0:
                    break;
                case REMOTE_BRANCH_INTERACTION:
                    if(last_input_branch)
                    {
                        create_remote_branch(last_input_branch);
                        break;
                    }
                    remote_branch_interation(&remote_branch);
                    break;
                case LOCAL_BRANCH_INTERACTION:
                    if(last_input_branch)
                    {
                        switch_branch(&local_branch, last_input_branch);
                        break;
                    }
                    local_branch_interaction(&local_branch);
                    break;
                default:
                    printf("unknown object bit\n");
                    break;
            }
        }

        goto exit;
    }

    for(size_t i=0; i<64; i++)
    {
        if(!feature_set)
            break;

        switch(get_and_reset_bit(&feature_set, 1<<i))
        {
            case 0:
                break;
            case HELP_MSG:
                print_help_msg();
                goto exit;
            case DELETE_BRANCH_INTERACTION:
                command_line_delete_branch(object_set, manipulate_target);
            case UPDATE_BRANCH_INFO:
                command_line_update_branch_info(object_set);
                break;
            default:
                fprintf(stderr, "unknown mark\n");
                break;
        }
    }

exit:

    /* clean resources*/
    for(size_t j=0; j<sizeof(branch_infos)/sizeof(BranchInfo*); j++)
    {
        for(size_t i=0; i<branch_infos[j]->branch_count; i++)
            if(branch_infos[j]->branch_index[i])
                free(branch_infos[j]->branch_index[i]);
        if(branch_infos[j]->branch_index)
            free(branch_infos[j]->branch_index);

        for(size_t i=0; i<branch_infos[j]->branch_count; i++)
            if(branch_infos[j]->branch_index_hint[i])
                free(branch_infos[j]->branch_index_hint[i]);
        if(branch_infos[j]->branch_index_hint)
            free(branch_infos[j]->branch_index_hint);

        if(branch_infos[j]->branch_index_hint_extra_size)
            free(branch_infos[j]->branch_index_hint_extra_size);

        if(branch_infos[j]->branch_operation_mark)
            free(branch_infos[j]->branch_operation_mark);

        if(branch_infos[j]->branch_location)
            free(branch_infos[j]->branch_location);
    }

    return;
}

int main(int argc, char **argv)
{
    size_t *feature_set =  NULL;
    size_t *object_set = NULL;
    char ***manipulate_target = NULL;
    size_t manipulate_count = 0;

    manipulate_count =
        parse_input_parameters(argc, argv, &object_set, &feature_set, &manipulate_target);

    for(size_t i=0; i<manipulate_count; i++)
        run_interaction(object_set[i], feature_set[i], manipulate_target[i]);

    if(manipulate_count == 0)
        run_interaction(0, 0, 0);

    free(feature_set);
    free(object_set);
    for(size_t i=0; i<manipulate_count; i++)
    {
        for(size_t j=0; manipulate_target[i][j] != (void*)UINT64_MAX; j++)
        {
            if(manipulate_target[i][j] == NULL)
                break;
            free(manipulate_target[i][j]);
        }
        free(manipulate_target[i]);
    }
    free(manipulate_target);

    return 0;
}


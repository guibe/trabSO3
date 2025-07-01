#include "filesystem.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main()
{
    Directory* current = get_root_directory();
    char command[1024];
    char cwd[1024] = "/";
    char* token;

    printf("Sistema de Arquivos com �rvore B\n");
    printf("Comandos: mkdir, touch, rm, rmdir, ls, cd, exit\n");

    while (1)
    {
        printf("%s> ", cwd);
        if (fgets(command, sizeof(command), stdin) == NULL)
            break;

        command[strcspn(command, "\n")] = 0;

        if (strlen(command) == 0)
            continue;

        token = strtok(command, " ");

        if (strcmp(token, "exit") == 0)
        {
            break;
        }
        else if (strcmp(token, "mkdir") == 0)
        {
            char* name = strtok(NULL, " ");
            if (name)
            {
                TreeNode* dir = create_directory(name);
                btree_insert(current->tree, dir);
                printf("Diret�rio criado: %s\n", name);
            }
            else
            {
                printf("Uso: mkdir <nome>\n");
            }
        }
        else if (strcmp(token, "touch") == 0)
        {
            char* name = strtok(NULL, " ");
            char* content = strtok(NULL, "\"");
            if (!content) content = strtok(NULL, "");

            if (name && content)
            {
                TreeNode* file = create_txt_file(name, content);
                if (file)
                {
                    btree_insert(current->tree, file);
                    printf("Arquivo criado: %s\n", name);
                }
            }
            else
            {
                printf("Uso: touch <nome> \"conte�do\"\n");
            }
        }
        else if (strcmp(token, "rm") == 0)
        {
            char* name = strtok(NULL, " ");
            if (name)
            {
                delete_txt_file(current->tree, name);
                printf("Arquivo removido: %s\n", name);
            }
            else
            {
                printf("Uso: rm <nome_arquivo>\n");
            }
        }
        else if (strcmp(token, "rmdir") == 0)
        {
            char* name = strtok(NULL, " ");
            if (name)
            {
                delete_directory(current->tree, name);
                printf("Diret�rio removido: %s\n", name);
            }
            else
            {
                printf("Uso: rmdir <nome_diretorio>\n");
            }
        }
        else if (strcmp(token, "ls") == 0)
        {
            printf("Conte�do de %s:\n", cwd);
            list_directory_contents(current);
        }
        else if (strcmp(token, "cd") == 0)
        {
            char* path = strtok(NULL, " ");
            if (path)
            {
                if (strcmp(path, "..") == 0)
                {
                    if (current != get_root_directory())
                    {
                        current = get_root_directory();
                        strcpy(cwd, "/");
                        printf("Diret�rio atual: /\n");
                    }
                }
                else
                {
                    change_directory(&current, path);
                    if (path[0] == '/')
                    {
                        strcpy(cwd, path);
                    }
                    else
                    {
                        if (strcmp(cwd, "/") != 0)
                            strcat(cwd, "/");
                        strcat(cwd, path);
                    }
                    printf("Diret�rio atual: %s\n", cwd);
                }
            }
            else
            {
                printf("Uso: cd <caminho>\n");
            }
        }
        else
        {
            printf("Comando inv�lido: %s\n", token);
            printf("Comandos v�lidos: mkdir, touch, rm, rmdir, ls, cd, exit\n");
        }
    }

    cleanup_fs();
    printf("Sistema encerrado.\n");
    return 0;
}

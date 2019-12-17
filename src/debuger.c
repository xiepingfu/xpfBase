#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "parser.h"
#include "utils.h"

#define MAX 4096

int main()
{
    char sqls[][MAX] = {
        "CREATE DATABASE school;",
        "USE school;",
        // "CREATE TABLE student(name STRING, age INT, grade FLOAT);",
        "CREATE TABLE teacher(name STRING, age INT, wage FLOAT);",
        // "INSERT INTO teacher(name, age) VALUES(t28, 28);",
        // "INSERT INTO teacher VALUES(t29, 29);",
        // "INSERT INTO teacher(name, age) VALUES(t30, 30);",
        // "INSERT INTO teacher VALUES(t31, 31);",
        // "SELECT name, age FROM student WHERE age=20;",
        // "CREATE INDEX ON student(age);",
        /* *
        "CREATE INDEX ON def(b);",
        "INSERT INTO student VALUES(1, xiepingfu, 22);",
        "INSERT INTO student(id, name, age) VALUES(2, xiepingfu, 22);",
        "INSERT INTO teacher(id, name) VALUES(3, ab);",
        "INSERT INTO teacher(id, name) VALUES(2, bc);",
        * */
        "",
    };
    XBParser xbParser;
    xbParser.clientfd = STDOUT_FILENO;
    for (int i = 0; strlen(sqls[i]) > 0; ++i)
    {
        printf("\n=====Test case %d:=====\n%s\n", i, sqls[i]);
        xbParser.sql = sqls[i];
        printf("\n=====Return code: %d=====\n", ParseSQL(&xbParser));
    }
    char insertSql[MAX];
    srand((unsigned)time(NULL));
    for (int i = 0; i < 1000; ++i)
    {
        int nameId = rand();
        int age = rand() % 50 + 25;
        float grade = rand() % 5000 + 5000;
        sprintf(insertSql, "INSERT INTO teacher(name, age, wage) VALUES(teacher%d, %d, %f);", nameId, age, grade);
        xbParser.sql = insertSql;
        printf("\n=====Return code: %d=====\n", ParseSQL(&xbParser));
    }
    char inputSql[MAX];

    while (read(STDIN_FILENO, inputSql, MAX) != -1)
    {
        xbParser.sql = inputSql;
        printf("\n=====Return code: %d=====\n", ParseSQL(&xbParser));
    }

    return 0;
}
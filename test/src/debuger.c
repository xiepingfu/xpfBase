#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "parser.h"
#include "utils.h"

#define MAX 4096

int main()
{
    return 0;
    char sqls[][MAX] = {
        "USE school;",
        "CREATE INDEX ON def(b);",
        /* *
        "INSERT INTO student VALUES(1, xiepingfu, 22);",
        "INSERT INTO student(id, name, age) VALUES(2, xiepingfu, 22);",
        "INSERT INTO teacher(id, name) VALUES(3, ab);",
        "INSERT INTO teacher(id, name) VALUES(2, bc);",
        * */
        "",
    };
    XBParser xbParser;
    for (int i = 0; strlen(sqls[i]) > 0; ++i)
    {
        printf("\n=====Test case %d:=====\n%s\n", i, sqls[i]);
        xbParser.sql = sqls[i];
        printf("\n=====Return code: %d=====\n", ParseSQL(&xbParser));
    }
    return 0;
}
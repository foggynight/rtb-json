#include "../rtb-json.h"

#include <stdio.h>

int main(void) {
    JSON *json = JSON_CreateArray();
    JSON_Delete(json);
    return 0;
}

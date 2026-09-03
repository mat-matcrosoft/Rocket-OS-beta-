#include "../api/api.h"
int main(void) {
    char buffer[256]; int input = rocket_open("README.TXT", 0); int output = rocket_open("COPY.TXT", 1);
    if (input < 0 || output < 0) { rocket_println("filecopy: open failed"); return 1; }
    int count = rocket_read(input, buffer, sizeof(buffer));
    if (count > 0) rocket_write(output, buffer, (uint32_t)count);
    rocket_println("filecopy: done"); return 0;
}

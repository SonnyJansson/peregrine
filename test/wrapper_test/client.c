#include "peregrine/logger_c.h"

void c_client_log() {
    Logger *main_a = root_get("main_a");
    Logger *main_b = root_get("main_b");
    Logger *main_a_sub_a = logger_get(main_a, "sub_a");
    Logger *main_b_sub_a = logger_get(main_b, "sub_a");

    log_info(main_a, "Hello, this is some info from main A");
    log_debug(main_b, "Hello, this is some debug info from main B");
    log_warning(main_a_sub_a, "I am not doing so well!");
    log_info(main_b_sub_a, "I am doing just fine!");
    log_critical(main_a, "Oops, dying now!");
}

#include <stdio.h>

struct mcw_server
{
    struct wl_display *wl_display;
    struct wl_event_loop *wl_event_loop;
};

int main(int argc, char **argv)
{
    printf("Hello Wardland\n");
}
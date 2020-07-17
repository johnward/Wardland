#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <wayland-server.h>
#include <wlr/backend.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/interfaces/wlr_output.h>

static void new_output_notify(struct wl_listener *listener, void *data);
static void output_destroy_notify(struct wl_listener *listener, void *data);
static void output_frame_notify(struct wl_listener *listener, void *data);

struct ward_server
{
    struct wl_display *wl_display;
    struct wl_event_loop *wl_event_loop;

    struct wlr_backend *backend;

    struct wl_listener new_output;
    struct wl_list outputs; // ward_output::link
};

struct ward_output
{
    struct wlr_output *wlr_output;
    struct ward_server *server;
    struct timespec last_frame;

    struct wl_listener frame;
    struct wl_listener destroy;

    struct wl_list link;
};

int main(int argc, char **argv)
{
    struct ward_server server;
    server.wl_display = wl_display_create();
    assert(server.wl_display);
    server.wl_event_loop = wl_display_get_event_loop(server.wl_display);
    assert(server.wl_event_loop);

    server.backend = wlr_backend_autocreate(server.wl_display, NULL);
    assert(server.backend);

    wl_list_init(&server.outputs);

    server.new_output.notify = new_output_notify;
    wl_signal_add(&server.backend->events.new_output, &server.new_output);

    if (!wlr_backend_start(server.backend))
    {
        fprintf(stderr, "Failed to start backend\n");
        wl_display_destroy(server.wl_display);
        return 1;
    }

    wl_display_run(server.wl_display);
    wl_display_destroy(server.wl_display);

    printf("Hello Wardland\n");
    return 0;
}

static void new_output_notify(struct wl_listener *listener, void *data)
{
    struct ward_server *server = wl_container_of(listener, server, new_output);
    struct wlr_output *woutput = data;

    if (!wl_list_empty(&woutput->modes))
    {
        struct wlr_output_mode *mode = wl_container_of(woutput->modes.prev, mode, link);
        wlr_output_set_mode(woutput, mode);
    }

    struct ward_output *output = calloc(1, sizeof(struct ward_output));
    clock_gettime(CLOCK_MONOTONIC, &output->last_frame);
    output->server = server;
    output->wlr_output = woutput;
    wl_list_insert(&server->outputs, &output->link);

    output->destroy.notify = output_destroy_notify;
    wl_signal_add(&woutput->events.destroy, &output->destroy);

    output->frame.notify = output_frame_notify;
    wl_signal_add(&woutput->events.frame, &output->frame);
}

static void output_destroy_notify(struct wl_listener *listener, void *data)
{
    struct ward_output *output = wl_container_of(listener, output, destroy);
    wl_list_remove(&output->link);
    wl_list_remove(&output->destroy.link);
    wl_list_remove(&output->frame.link);
    free(output);
}

static void output_frame_notify(struct wl_listener *listener, void *data)
{
    struct ward_output *output = wl_container_of(listener, output, frame);
    struct wlr_output *wlr_output = data;

    struct wlr_renderer *renderer = wlr_backend_get_renderer(wlr_output->backend);

    wlr_output_attach_render(wlr_output, NULL);
    wlr_renderer_begin(renderer, wlr_output->width, wlr_output->height);

    float color[4] = {1.0, 0, 0, 1.0};
    wlr_renderer_clear(renderer, color);

    //wlr_output_swap_buffers(wlr_output, NULL, NULL);  // Old API

    /* Conclude rendering and swap the buffers, showing the final frame
	 * on-screen. */
    wlr_renderer_end(renderer);
    wlr_output_commit(output->wlr_output);
}
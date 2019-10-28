#include <glib.h>
#include <gio/gio.h>
#include <stdlib.h>

void countdown(void)
{
    for(gsize i = 5 ; i >= 1 ; i--)
    {
        g_print("%" G_GSIZE_FORMAT "\n", i);
        g_usleep(1 * G_USEC_PER_SEC);
    }
}

void dieIfRemote(GApplication *App)
{
    if(g_application_get_is_remote(App))
    {
        g_printerr("Already running.\n");
        exit(EXIT_FAILURE);
    }
}

void activate(GApplication *App)
{
    dieIfRemote(App);

    g_print("I'm activated !\n");
    countdown();
}

void startup(GApplication *App)
{
    dieIfRemote(App);

    g_print("I'm started !\n");
    countdown();
}

int main(int argc, char *argv[])
{
    GApplication *App;
    gint Status;

    App = g_application_new("io.myapp.test", 0);
    g_signal_connect(App, "activate", G_CALLBACK(activate), NULL);
    g_signal_connect(App, "startup", G_CALLBACK(startup), NULL);

    g_application_register(App, NULL, NULL);

    dieIfRemote(App);

    Status = g_application_run(G_APPLICATION(App), argc, argv);
    g_object_unref(App);

    return Status;
}

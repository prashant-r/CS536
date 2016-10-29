#include <gtk/gtk.h>

GtkBuilder *builder;
GObject *login_window;

void init_gui(int *argc, char **argv[]);
void event_loop();
void create_message_dialog(const char *message);

void init_gui(int *argc, char **argv[]) {
    
    gtk_init(argc, argv);
    
    builder = gtk_builder_new();
 
    guint build_error = gtk_builder_add_from_file(builder, "chat_window.ui", NULL);
    
    login_window = gtk_builder_get_object(builder, "chat_window");
    
    g_signal_connect(login_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    
    
}

void event_loop() {
   gtk_main();
}

void create_message_dialog(const char *message) {

   GtkWidget *message_dialog;
   
   message_dialog = gtk_message_dialog_new ((GtkWindow*)login_window,
                                 GTK_DIALOG_DESTROY_WITH_PARENT,
                                 GTK_MESSAGE_ERROR,
                                 GTK_BUTTONS_CLOSE,
                                 message);
   gtk_dialog_run (GTK_DIALOG (message_dialog));
   gtk_widget_destroy (message_dialog);

}

static void button_clicked(GtkWidget *widget, gpointer data) {

    g_print("Hello! \n");

}

int main(int argc, char *argv[]) {
   
   GtkBuilder *builder;
   GtkWidget *window;
   GtkWidget *button;
   
   gtk_init(&argc, &argv);
   
   // Widget creation: Main window
   window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title(GTK_WINDOW(window), "Tiny chat");
   g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
   
   // Widget creation: button   
   button = gtk_button_new_with_label("Click!");
   g_signal_connect(button, "clicked", G_CALLBACK(button_clicked), NULL);
   gtk_container_add(GTK_CONTAINER(window), button);

   gtk_widget_show(button);
   gtk_widget_show(window);
  
   gtk_main();
  
   return 0;
 
}
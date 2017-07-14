#include <iostream>
#include <jack/types.h>
#include <jack/jack.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <cstring>

#include <config.h>

typedef jack_default_audio_sample_t sample_t;

struct jack_sampler {
  jack_port_t* output_port1;
  jack_port_t* output_port2;
};

int process_callback(jack_nframes_t nframes, void* arg) {
  jack_sampler* sampler = (jack_sampler*) arg;
  sample_t* buffer1 = (sample_t*) jack_port_get_buffer(sampler->output_port1, nframes);
  sample_t* buffer2 = (sample_t*) jack_port_get_buffer(sampler->output_port2, nframes);
  memset(buffer1, 0, sizeof(sample_t) * nframes);
  memset(buffer2, 0, sizeof(sample_t) * nframes);
  return 0;
}

int main() {
  jack_client_t* client;

  // init jack
  jack_status_t status;
  if ((client = jack_client_open("jmage-sampler", JackNullOption, &status)) == NULL) {
    std::cerr << "failed to open jack client" << std::endl;
    return 1;
  }

  jack_sampler* sampler = new jack_sampler;

  jack_set_process_callback(client, process_callback, sampler);
  sampler->output_port1 = jack_port_register(client, "out1", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  sampler->output_port2 = jack_port_register(client, "out2", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

  if (jack_activate(client)) {
    jack_port_unregister(client, sampler->output_port1);
    jack_port_unregister(client, sampler->output_port2);
    jack_client_close(client);
    delete sampler;
    std::cerr <<"cannot activate jack client" << std::endl;
    return 1;
  }
  
  int from_child_pipe[2];
  int to_child_pipe[2];

  pipe(from_child_pipe);
  pipe(to_child_pipe);

  pid_t pid = fork();

  // i'm the child
  if (pid == 0) {
    dup2(from_child_pipe[1], 1);
    close(from_child_pipe[0]);

    dup2(to_child_pipe[0], 0);
    close(to_child_pipe[1]);

    execl(CONFIG_INSTALL_PREFIX "/libexec/jm-sampler-ui", "jm-sampler-ui", NULL);
  }
  // i'm the parent
  close(from_child_pipe[1]);
  close(to_child_pipe[0]);

  // close fd's subsequent children don't need on exec
  fcntl(from_child_pipe[0], F_SETFD, FD_CLOEXEC); 
  // this fd is important to close on exec so child properly gets EOF when parent closes fout
  fcntl(to_child_pipe[1], F_SETFD, FD_CLOEXEC); 

  FILE* fin = fdopen(from_child_pipe[0], "r");
  FILE* fout = fdopen(to_child_pipe[1], "w");

  char buf[256];

  while (fgets(buf, 256, fin) != NULL) {
    std::cout << "UI: " << buf;
  }

  fclose(fout);
  waitpid(pid, NULL, 0);

  jack_deactivate(client);
  jack_port_unregister(client, sampler->output_port1);
  jack_port_unregister(client, sampler->output_port2);
  jack_client_close(client);
  delete sampler;

  return 0;
}

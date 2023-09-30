#include "pipe/graph.h"
#include "pipe/global.h"
#include "core/log.h"
#include "qvk/qvk.h"

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
  const char *cfg = 0;
  if(argc > 1) cfg = argv[1];
  else
  {
    fprintf(stderr, "please give a graph config as parameter\n");
    exit(1);
  }
  dt_log_init(s_log_cli|s_log_pipe);
  dt_log_init_arg(argc, argv);
  dt_pipe_global_init();
  if(qvk_init()) exit(1);
  dt_graph_t graph;
  dt_graph_init(&graph);
  int err = dt_graph_read_config_ascii(&graph, cfg);
  assert(!err);
  // TODO: perform some more exhaustive consistency checks
  // output dot file, build with
  // ./tests/graph > graph.dot
  // dot -Tpdf graph.dot -o graph.pdf
  fprintf(stdout, "digraph M {\n");
  // for all nodes, print all incoming edges (outgoing don't have module ids)
  for(int m=0;m<graph.num_modules;m++)
  {
    for(int c=0;c<graph.module[m].num_connectors;c++)
    {
      if((graph.module[m].connector[c].type == dt_token("read") ||
          graph.module[m].connector[c].type == dt_token("sink")) &&
          graph.module[m].connector[c].connected_mid >= 0)
      {
        fprintf(stdout, "%"PRItkn" -> %"PRItkn"\n",
            dt_token_str(graph.module[
            graph.module[m].connector[c].connected_mid
            ].name),
            dt_token_str(graph.module[m].name));
      }
    }
  }
  fprintf(stdout, "}\n");


  dt_graph_run(&graph, s_graph_run_all);
  // TODO: debug rois
  fprintf(stdout, "digraph N {\n");
  // for all nodes, print all incoming edges (outgoing don't have module ids)
  for(int m=0;m<graph.num_nodes;m++)
  {
    for(int c=0;c<graph.node[m].num_connectors;c++)
    {
      if((graph.node[m].connector[c].type == dt_token("read") ||
          graph.node[m].connector[c].type == dt_token("sink")) &&
          graph.node[m].connector[c].connected_mid >= 0)
      {
        fprintf(stdout, "%"PRItkn" -> %"PRItkn"\n",
            dt_token_str(graph.node[
            graph.node[m].connector[c].connected_mid
            ].name),
            dt_token_str(graph.node[m].name));
      }
    }
  }
  fprintf(stdout, "}\n");

  dt_graph_cleanup(&graph);
  dt_pipe_global_cleanup();
  qvk_cleanup();
  exit(0);
}

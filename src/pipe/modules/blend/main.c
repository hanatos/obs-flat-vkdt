#include "modules/api.h"
#include <stdio.h>

dt_graph_run_t
check_params(
    dt_module_t *module,
    uint32_t     parid,
    uint32_t     num,
    void        *oldval)
{
  if(parid == 2)
  { // mode
    int oldmode = *(int*)oldval;
    int newmode = dt_module_param_int(module, parid)[0];
    if(oldmode != newmode) //  would need to test one of them is 2, 4 or 5
      return s_graph_run_all;
  }
  return s_graph_run_record_cmd_buf; // minimal parameter upload to uniforms
}

void create_nodes(dt_graph_t *graph, dt_module_t *module)
{
  const int32_t p_mode = dt_module_param_int(module, dt_module_get_param(module->so, dt_token("mode")))[0];
  dt_roi_t roif = module->connector[1].roi; // roi of input (it's = output)
  if(p_mode == 2 || p_mode == 4 || p_mode == 5)
  { // focus stack or exposure fusion or pyramidal blend modes
    dt_roi_t roic = roif;
    const int maxnuml = 14;
    const int loff = log2f(roif.full_wd / roif.wd);
    const int numl = maxnuml-loff;
    int id_down0[maxnuml], id_down1[maxnuml], id_up[maxnuml];
    for(int l=0;l<numl;l++)
    {
      const int pc[] = { l, numl };
      roic.wd = (roif.wd+1)/2;
      roic.ht = (roif.ht+1)/2;
      id_down0[l] = dt_node_add(graph, module, "blend", "down",
          roic.wd, roic.ht, 1, sizeof(pc), pc, 3,
          "input",  "read",  "rgba", "f16", dt_no_roi,
          "output", "write", "rgba", "f16", &roic,
          "mask",   "read",  "*",    "*",   dt_no_roi);
      id_down1[l] = dt_node_add(graph, module, "blend", "down",
          roic.wd, roic.ht, 1, sizeof(pc), pc, 3,
          "input",  "read",  "rgba", "f16", dt_no_roi,
          "output", "write", "rgba", "f16", &roic,
          "mask",   "read",  "*",    "*",   dt_no_roi);
      id_up[l] = dt_node_add(graph, module, "blend", "up",
          roif.wd, roif.ht, 1, sizeof(pc), pc, 6,
          "coarse0", "read",  "rgba", "f16", dt_no_roi,
          "fine0",   "read",  "rgba", "f16", dt_no_roi,
          "coarse1", "read",  "rgba", "f16", dt_no_roi,
          "fine1",   "read",  "rgba", "f16", dt_no_roi,
          "co",      "read",  "rgba", "f16", dt_no_roi,
          "output",  "write", "rgba", "f16", &roif);
      roif = roic;
    }
    dt_connector_copy(graph, module, 0, id_down0[0], 0);
    dt_connector_copy(graph, module, 1, id_down1[0], 0);
    dt_connector_copy(graph, module, 2, id_down0[0], 2);
    dt_connector_copy(graph, module, 2, id_down1[0], 2);
    dt_connector_copy(graph, module, 3, id_up[0],    5); // output
    dt_connector_copy(graph, module, 0, id_up[0],    1);
    dt_connector_copy(graph, module, 1, id_up[0],    3);
    for(int l=1;l<numl;l++)
    {
      dt_connector_copy(graph, module, 2, id_down0[l], 2); // pass on mask just to have something connected (not read)
      dt_connector_copy(graph, module, 2, id_down1[l], 2);
      dt_node_connect(graph, id_down0[l-1], 1, id_down0[l], 0); // downsize more
      dt_node_connect(graph, id_down1[l-1], 1, id_down1[l], 0);
      dt_node_connect(graph, id_down0[l-1], 1, id_up[l],    1); // fine for details during upsizing
      dt_node_connect(graph, id_down1[l-1], 1, id_up[l],    3);
      dt_node_connect(graph, id_down0[l-1], 1, id_up[l-1],  0); // coarse
      dt_node_connect(graph, id_down1[l-1], 1, id_up[l-1],  2);
      dt_node_connect(graph, id_up[l],      5, id_up[l-1],  4); // pass on to next level
    }
    dt_node_connect(graph, id_down0[numl-1], 1, id_up[numl-1], 0); // coarsest level
    dt_node_connect(graph, id_down1[numl-1], 1, id_up[numl-1], 2);
    dt_node_connect(graph, id_down0[numl-1], 1, id_up[numl-1], 4); // use coarsest from back buffer
  }
  else
  { // per pixel blending, all the same kernel
    int pc[] = {module->img_param.filters};
    int id_main = dt_node_add(graph, module, "blend", "main",
        roif.wd, roif.ht, 1, sizeof(pc), pc, 4,
        "back",   "read",  "rgba", "f16", dt_no_roi,
        "top",    "read",  "rgba", "f16", dt_no_roi,
        "mask",   "read",  "r",    "f16", dt_no_roi,
        "output", "write", "rgba", "f16", &roif);
    for(int k=0;k<4;k++) dt_connector_copy(graph, module, k, id_main, k);
  }
}

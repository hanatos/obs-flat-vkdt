PIPE_O=\
pipe/alloc.o\
pipe/connector.o\
pipe/global.o\
pipe/graph.o\
pipe/graph-io.o\
pipe/graph-export.o\
pipe/module.o\
pipe/raytrace.o
PIPE_H=\
core/fs.h\
pipe/alloc.h\
pipe/connector.h\
pipe/connector.inc\
pipe/cycles.h\
pipe/dlist.h\
pipe/draw.h\
pipe/global.h\
pipe/graph.h\
pipe/graph-run-modules.h\
pipe/graph-run-nodes-allocate.h\
pipe/graph-run-nodes-upload.h\
pipe/graph-run-nodes-record-cmd.h\
pipe/graph-run-nodes-download.h\
pipe/graph-io.h\
pipe/graph-print.h\
pipe/graph-export.h\
pipe/graph-traverse.inc\
pipe/modules/api.h\
pipe/asciiio.h\
pipe/module.h\
pipe/node.h\
pipe/params.h\
pipe/pipe.h\
pipe/raytrace.h\
pipe/token.h
PIPE_CFLAGS=
PIPE_LDFLAGS=-ldl

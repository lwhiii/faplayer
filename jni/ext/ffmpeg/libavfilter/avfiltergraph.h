/*
 * Filter graphs
 * copyright (c) 2007 Bobby Bingham
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef AVFILTER_AVFILTERGRAPH_H
#define AVFILTER_AVFILTERGRAPH_H

#include "avfilter.h"

typedef struct AVFilterGraph {
    unsigned filter_count;
    AVFilterContext **filters;

    char *scale_sws_opts; ///< sws options to use for the auto-inserted scale filters
} AVFilterGraph;

/**
 * Allocate a filter graph.
 */
AVFilterGraph *avfilter_graph_alloc(void);

/**
 * Get a filter instance with name name from graph.
 *
 * @return the pointer to the found filter instance or NULL if it
 * cannot be found.
 */
AVFilterContext *avfilter_graph_get_filter(AVFilterGraph *graph, char *name);

/**
 * Add an existing filter instance to a filter graph.
 *
 * @param graphctx  the filter graph
 * @param filter the filter to be added
 */
int avfilter_graph_add_filter(AVFilterGraph *graphctx, AVFilterContext *filter);

/**
 * Check for the validity of graph.
 *
 * A graph is considered valid if all its input and output pads are
 * connected.
 *
 * @return 0 in case of success, a negative value otherwise
 */
int avfilter_graph_check_validity(AVFilterGraph *graphctx, AVClass *log_ctx);

/**
 * Configure all the links of graphctx.
 *
 * @return 0 in case of success, a negative value otherwise
 */
int avfilter_graph_config_links(AVFilterGraph *graphctx, AVClass *log_ctx);

/**
 * Configure the formats of all the links in the graph.
 */
int avfilter_graph_config_formats(AVFilterGraph *graphctx, AVClass *log_ctx);

/**
 * Check validity and configure all the links and formats in the graph.
 *
 * @see avfilter_graph_check_validity(), avfilter_graph_config_links(),
 * avfilter_graph_config_formats()
 */
int avfilter_graph_config(AVFilterGraph *graphctx, AVClass *log_ctx);

/**
 * Free a graph and destroy its links.
 */
void avfilter_graph_free(AVFilterGraph *graph);

/**
 * A linked-list of the inputs/outputs of the filter chain.
 *
 * This is mainly useful for avfilter_graph_parse(), since this
 * function may accept a description of a graph with not connected
 * input/output pads. This struct specifies, per each not connected
 * pad contained in the graph, the filter context and the pad index
 * required for establishing a link.
 */
typedef struct AVFilterInOut {
    /** unique name for this input/output in the list */
    char *name;

    /** filter context associated to this input/output */
    AVFilterContext *filter_ctx;

    /** index of the filt_ctx pad to use for linking */
    int pad_idx;

    /** next input/input in the list, NULL if this is the last */
    struct AVFilterInOut *next;
} AVFilterInOut;

/**
 * Add a graph described by a string to a graph.
 *
 * @param graph   the filter graph where to link the parsed graph context
 * @param filters string to be parsed
 * @param inputs  linked list to the inputs of the graph
 * @param outputs linked list to the outputs of the graph
 * @return zero on success, a negative AVERROR code on error
 */
int avfilter_graph_parse(AVFilterGraph *graph, const char *filters,
                         AVFilterInOut *inputs, AVFilterInOut *outputs,
                         AVClass *log_ctx);

#endif  /* AVFILTER_AVFILTERGRAPH_H */
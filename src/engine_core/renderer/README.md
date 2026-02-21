# Hush renderer

This module contains renderers for Hush.

Each renderer implements a set of passes that are executed by the RenderGraph and provide an output
texture that can be used by other passes or presented to the screen.

Currently, it only provides a renderer that has a pass for geometry rendering, a pass for
drawing lights (currently, only directional lights), and a pass to combine the results of the previous two passes and output the final image.

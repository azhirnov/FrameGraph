Graph visualization contains:
* Initial and final state of all resources that will be used in current subbatch.
* All pipeline barrier that automaticaly placed between tasks.
* All tasks with default or custom name and color.
* Synchronizations and memory transfer between host and device.

To set task name and color use `SetName()` and `SetDebugColor()`.

Setup command buffer for debugging:
```cpp
CommandBufferDesc desc;
desc.SetDebugFlags( EDebugFlags::Default );

CommandBuffer cmdbuf = frameGraph->Begin( desc );
```

Use `IFrameGraph::DumpToGraphViz` to retrive graph description in dot-language.<br/>
Or use [GraphViz helper library](../extensions/graphviz) to retrive and visualize graph with [graphviz](https://www.graphviz.org/) (should be installed).

Example:
![image](FrameGraph1.png)

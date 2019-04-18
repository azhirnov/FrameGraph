Warning: this is just an example, something may change, so see [tests](../tests/framegraph) or [samples](../samples) for currently working code.


## Validation
When you developing multithreaded renderer using FrameGraph be sure that macro `FG_ENABLE_DATA_RACE_CHECK` is defined. This will allow you to immediately detect the most of the problems with race conditions (missed syncs).


## CPU thread synchronization examples
```cpp
Barrier        sync {2};
CommandBuffer  mainCmdBuf;

void RenderThread1 (FrameGraph fg)
{
	CommandBuffer  submittedCmdBuffers[2];

	for (uint frame_id = 0;; ++frame_id)
	{
		CommandBuffer& cmdbuf = submittedCmdBuffers[frame_id&1];
		
		// for double buffering
		fg->Wait({ cmdbuf });
		
		cmdbuf = fg->Begin( CommandBufferDesc{ EQueueType::Graphics });
		
		// (1) notify 'thread-2' that new frame started
		sync.wait();
		
		// access to 'mainCmdBuf' protected by 'sync' barrier
		mainCmdBuf = cmdbuf;
		
		// append to pending command buffers
		fg->Execute( cmdbuf );
		
		// (2) wait until 'thread-2' executed command buffer
		sync.wait();
		
		// submit command buffers from 'thread-1' and 'thread-2'
		fg->Flush();
	}
}

void RenderThread2 (FrameGraph fg)
{
	for (;;)
	{
		CommandBuffer cmdbuf = fg->Begin( CommandBufferDesc{ EQueueType::Graphics });
		
		// (1) wait for new frame
		sync.wait();
			
		// submit current command buffer after 'mainCmdBuf'
		// access to 'mainCmdBuf' protected by 'sync' barrier
		cmdbuf->AddDependency( mainCmdBuf );
		
		// append to pending command buffers
		fg->Execute( cmdbuf );
		
		// (2) notify 'thread-1' that command buffer was executed
		sync.wait();
	}
}
```

```
.------------.------------.
|  thread_1  |  thread_2  |
|------------|------------|
|   Begin    |            | - 'Begin' is internally synchronized
|            |   Begin    |
|------------|------------|
|  sync (1)  |  sync (1)  | - barrier
|------------|------------|
|    ...     |    ...     |
|  Execute   |    ...     |
|            |  Execute   | - 'Execute' is internally synchronized
|------------|------------|
|  sync (2)  |  sync (2)  | - barrier
|------------|------------|
|   Flush    |            | - 'Flush' is internally synchronized
'------------'------------'
```


## Resource creation and destruction.
```cpp
auto image = fg->CreateImage( ... );

auto cmdbuf = fg->Begin( ... );
cmdbuf->AddTask( CopyColorImage{}.From( ... ).To( image ) ... );  // 'image' will be captuted by the command buffer
fg->Execute( cmdbuf );

fg->ReleaseResource( image );  // release reference for the 'image'

fg->Wait({ cmdbuf });  // image will be destroyed here
```

```cpp
auto image = fg->CreateImage( ... );

fg->ReleaseResource( image );  // immediately destroy 'image'
```
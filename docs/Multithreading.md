Warning: this is just an example, something may change, so see [tests](../tests/framegraph) or [samples](../samples/framegraph) for currently working code.

## Validation
When you developing multithreaded renderer using FrameGraph be sure that macro `FG_ENABLE_RACE_CONDITION_CHECK` is defined. This will allow you to immediately detect the most of the problems with race conditions (missed syncs).


## CPU thread synchronization examples
FrameGraph have some internal almost always lock-free synchronizations and requires some external synchronizations. Here we see how to implement it.
1. With separate render threads.
```
.------------.------------.------------.------------.------------.
|  thread_1  |  thread_2  |  thread_3  |  thread_4  |  thread_5  |
|------------|------------|------------|------------|------------|
|                        BeginFrame                              |
|                     cv.notify_all - wake up all threads        |
|------------|------------|------------|------------|------------|
|   Begin    |            |   Begin    |            |            |
|    ###     |   Begin    |    ###     |            |   Begin    |
|  Execute   |    ###     |    ###     |   Begin    |    ###     |
|            |  Execute   |  Execute   |    ###     |    ###     |
|            |            |            |  Execute   |  Execute   |
|------------|------------|------------|------------|------------|
|                        barrier - wait for all threads          |
|                        EndFrame                                |
'------------'------------'------------'------------'------------'
```
2. With job system to maximize CPU workload.
```
.------------.------------.------------.------------.------------.
|  thread_1  |  thread_2  |  thread_3  |  thread_4  |  thread_5  |
|------------|------------|------------|------------|------------|
| BeginFrame | job ended  |     ...    |     ...    |    ...     | - insert render tasks into job system
|   Begin    |   Begin    | job ended  |     ...    | job ended  |   using lock-free algorithm.
|    ###     |    ###     |   Begin    | job ended  |   Begin    |
|  Execute   |    ###     |    ###     |   Begin    |    ###     |
|   Begin    |  Execute   |  Execute   |    ###     |    ###     |
|    ###     | begin job  |   Begin    |    ###     |  Execute   | - atomicaly set bit that indicates 
|  Execute   |    ...     |    ###     |  Execute   | begin job  |   that render task is complete.
|  waiting   |    ...     |  Execute   | begin job  |    ...     |
|  EndFrame  |    ...     | begin job  |    ...     |    ...     | - blocks only one thread to wait until all
'------------'------------'------------'------------'------------'   render tasks are complete.
```

## Resource creation and destruction.
It is prefered to create and destroy resources in asynchronous mode.
```
.------------.------------.------------.------------.------------.
|  thread_1  |  thread_2  |  thread_3  |  thread_4  |  thread_5  |
|------------|------------|------------|------------|------------|
|   Create*  |                                                   | - in synchronous mode only one thread can
|   Create*  |                                                   |   create and destroy resources.
| BeginFrame |                                                   |
|------------|------------|------------|------------|------------|
|   Begin    |   Begin    |   Begin    |   Begin    |   Begin    |
|  Create*   |  Create*   |  Create*   |  Create*   |  Create*   | - in asynchronous mode resource can be
|    ###     |  Create*   |    ###     |    ###     |    ###     |   created at any thread without synchronizations
|  Release*  |    ###     |    ###     |    ###     |    ###     |   and internally there is wait-free algorithm to
|  Execute   |  Execute   |  Execute   |  Execute   |  Execute   |   acquire ID for resource.
|------------|------------|------------|------------|------------|
|  EndFrame  |                                                   |
|  Release*  |                                                   |
|  Release*  |                                                   |
'------------'------------'------------'------------'------------'
```
<br/>

In synchronous mode

```
auto image = fg->CreateImage( ... );
...
fg->ReleaseResource( image );   // image destroed immediatly
 ```
In asynchronous mode

```
auto image = fg->CreateImage( ... );
...
fg->ReleaseResource( image );  // just released reference to the image
...
EndFrame() // image destroed here
```

Warning: all Vulkan resources will be destroyed with delay to be sure that GPU has finished using the resource.
`ReleaseResource` method destoy only the FrameGraph wrapper for that resource.


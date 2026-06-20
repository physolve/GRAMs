# Qt TaskTree Patterns

## Минимальный пример TaskTree
```cpp
#include <tasking/tasktree.h>
#include <tasking/concurrentcall.h>

using namespace Tasking;

const Group root {
    ConcurrentCallTask<int>([](ConcurrentCall<int> &task) {
        task.setConcurrentCallData([]() { return 42; });
    })
};

QTaskTree *tree = new QTaskTree(root, this);
connect(tree, &QTaskTree::done, this, [](DoneWith result) {
    qDebug() << (result == DoneWith::Success ? "OK" : "FAIL");
});
tree->start();
```

# DBS-Testcase

## 使用说明

用来本地评测的入口文件是 `runner.py`，使用时你理应在本文件夹下启动评测器，将你编译好的程序的相对路径或者绝对路径复制到下面的参数中，参考指令如下：

```bash
python3 runner.py -f query data -- /path/to/your/prog
```

注意:

- 用 `--` 是为了将 `runner.py` 的参数与你的 `prog` 参数隔开，在你的程序后面的参数不会被传给 `runner.py`，例如如果指令写成 `python3 runner.py -- /path/to/your/prog -f query data`，则 `runner.py` 不会收到 `-f` 参数。
-  部分测例会有 `flag` 标记，表明该测例只有给出 flag 后才会显式地启动该测例，上述例子中便启用了两个 flag，分别是 `query` 和 `data`，如果你不希望启用任何 flag，则可以不使用 `-f` 参数，或者提供无效参数如 `-f no_use`。

在 CI 评测时为了执行编译流程并解决难以找到自己可执行程序的路径的问题，另外编写了一个 `run-ci.py` 进行上层封装，它被期望在你自己的项目中运行，会读取你项目根目录下的 `testcase.yml` 配置文件并据此进行操作：
 
0. 验证 CI 文件的 hash
1. 以你的项目根目录为工作目录，根据配置文件 `compile` 段进行编译
2. 以 `dbs-testcase` 的根目录为工作目录，运行 `runner.py`，但是在启用你的程序时会将工作目录切换回你的项目根目录

例如你的项目在 `/path/to/student-repo`，本仓库的路径在 `/path/to/dbs-testcase`，那么如下是一个可能的运行`testcase.yml`配置

```yml
compile:
  commands: make

run:
  commands: ./bin/mydb
  flags:
    - no_use
```

然后你在 `/path/to/student-repo` 中执行 

```bash
python3 /path/to/dbs-testcase/run-ci.py
```

简单来说你不必研究 `run-ci.py` 的工作原理，你只需要记住在你自己项目的 `dbs-testcase.yml` 中的 `commands` 都是以你项目的根目录为参考即可。

考虑到部分同学可能不熟悉 `yaml` 格式，这里再给出一个更详细的关于 `flags` 的例子：

```yml

compile:
  commands: make

run:
  commands: ./bin/mydb
  flags:
    - query
    - data
    - "null"
```

在这一例子中，一方面要注意多个 `flag` 的指定方法，另一方面由于在 `yaml` 文件中 `null` 作为关键字会被解释为空值，因此需要显式地加引号来表明这是一个字符串。

## 指定测例

有两种情况下你可能希望指定几个测试点进行运行:

1. 你自己编写了测例和对应的答案，你希望只运行这几个测例
2. 你加载了数据集 DATASET 后希望基于此进行查询，不要花数分钟至数十分钟加载一遍再运行

为了支持这样的需求，提供了 `-c` 参数，你可以在后面加上空格隔开的若干个**测例名**来指定评测器无视依赖关系、flag 等约束直接运行这几个测例，顺序与你给出的测例名顺序一致。例如
```bash
python3 runner.py -c query data -- /path/to/your/prog
```

## 数据说明

注意，由于评测器的设计问题，测例有一些需要考虑的约定：

1. 使用 `ORDER BY` 时，应该将排序字段也输出到结果中，否则评测器会因为无法判定该部分的有序性而直接视为结果正确。例如 `SELECT name FROM T ORDER BY id`，评测器得到的参考答案和用户输出都只有一列 `name`，那么评测器只能认为所有行的 `id` 都是一样的，用户以任意顺序输出均为正确。
2. 主外键的创建不应该与显式索引有冲突，否则很难规定 DESC 的显示内容。
3. 插入的数据中最好不要包含 `@` 符号，如果包含最好不要是字符串的第一个字符，如果是则绝不能有查询使得 `@` 出现在行首，否则这会被评测器识别为输出内容结束的标志。
4. 使用 `SELECT *` 时不应该再包含其他字段的查询，因为检查器为了处理 `SELECT *` 会构造表头映射，此时重复的字段名会导致检查器错误。注意我们在批处理模式的文档中也约定了 `SELECT *` 只用于单表查询。

## 标程说明

附带了 `std/main.py` 作为借用 MySQL 跑测例的标程，附带的第三方库放在 `std/requirements.txt` 中，可以用于生成答案，参考运行方式如下：

```bash
python runner.py --std -f [flags, ...] -- python std/main.py <args...>
```

注意标程对测例进行了高度定制化，包含许多并不优雅的实现，它只需要对于标准测例能给出正确答案，试图用自定义测例hack标程是毫无意义的。

## BUG声明

本评测器于2023年秋季学期开发，因时间仓促没能经过充分测试，难免会有BUG。实验动手较早的同学可能在使用过程中遇到若干评测器BUG，属于正常情况，将这些问题反馈给助教或者直接提交 PR 都有机会得到大作业加分，详见线上文档-CI说明-激励机制。

注：提交 issue、PR 等请在 Github 仓库操作，GitLab 仓库仅作为校内 mirror。

## TODO

以下是优先级不高，可能在将来学期完成的内容：

- 用 `logger` 库重构输出

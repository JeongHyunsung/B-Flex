# Output Format Spec (`part` / `status.txt`)

This document defines the current output format for B-Flex execution results.

Scope

- `solutions_out/*.txt` or `*.txt.gz`
- `solutions_out/*.status.txt`

## 1. Terminology

- `group`
  - A branch unit that reaches `parallel_branch_depth`
- `part file`
  - A file storing a chunk of solutions for one group/task (`_part_<N>`)
- `status file`
  - Group-level progress metadata file (`.status.txt`)

## 2. Part File Naming

### 2.1 Group-level task

```text
<group_key>_part_<index>.txt
<group_key>_part_<index>.txt.gz
```

Example:

```text
L4_0_0_1_2_part_0.txt.gz
```

### 2.2 Non-group-level task (fallback naming)

```text
group_<group_id>_task_<task_id>_part_<index>.txt
group_<group_id>_task_<task_id>_part_<index>.txt.gz
```

## 3. Part File Content Format

Each line = one `truth_table` (one solution)

Format:

```text
<v0> <v1> <v2> ... <vN-1>\n
```

Characteristics

- Values are separated by spaces
- A trailing space may appear before `\n` (this is valid)
- Values are decimal integers
- The `DONT_CARE` sentinel is emitted as `65535` (`0xFFFF`)

### Example (plain text)

```text
3 1 65535 2 2 0 7 7 
0 0 1 1 2 2 3 3 
```

## 4. Status File Naming

```text
<group_key>.status.txt
```

Example:

```text
L4_0_0_1_2.status.txt
```

## 5. Status File Content Format

Current keys (2):

```text
solutions:<uint64>
done:<0|1>
```

Example:

```text
solutions:1842
done:1
```

Meaning

- `solutions`
  - Cumulative number of solutions flushed for this group
- `done`
  - `1`: group computation completed
  - `0`: incomplete / restart required

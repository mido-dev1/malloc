# Virtual address space of a process:
- get the process (by `pidof`)
- run: `cat /proc/$PID/maps` (see [proc(5)](https://linux.die.net/man/5/proc))

# Sources:

- Malloc implementation example: [https://silent-tower.net/projects/visual-overview-malloc](https://silent-tower.net/projects/visual-overview-malloc)

- Some good informations:
    - [https://gabrieletolomei.wordpress.com/miscellanea/operating-systems/in-memory-layout/](https://gabrieletolomei.wordpress.com/miscellanea/operating-systems/in-memory-layout/)

    - [https://gabrieletolomei.wordpress.com/miscellanea/operating-systems/virtual-memory-paging-and-swapping/](https://gabrieletolomei.wordpress.com/miscellanea/operating-systems/virtual-memory-paging-and-swapping/)

- Malloc max: [https://stackoverflow.com/questions/3463207/how-big-can-a-malloc-be-in-c](https://stackoverflow.com/questions/3463207/how-big-can-a-malloc-be-in-c)

- Glibc malloc wiki: [https://sourceware.org/glibc/wiki/MallocInternals](https://sourceware.org/glibc/wiki/MallocInternals)

- Good malloc explaination: [https://sploitfun.wordpress.com/2015/02/10/understanding-glibc-malloc/](https://sploitfun.wordpress.com/2015/02/10/understanding-glibc-malloc/)

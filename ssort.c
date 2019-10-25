#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <limits.h>

#include "float_vec.h"
#include "barrier.h"
#include "utils.h"

int compar(const void * a, const void * b) {
    float v1 = *((const float *)a);
    float v2 = *((const float *)b);
    return (v1 > v2) - (v1 < v2);
}
void
qsort_floats(floats* xs)
{
    // TODO: call qsort to sort the array
    // see "man 3 qsort" for details
    qsort(xs->data, xs->size, sizeof(float), compar);
}

floats*
sample(float* data, long size, int P)
{
    // TODO: sample the input data, per the algorithm decription
    int num = 3 * (P - 1);
    float samp[num];
    for(int i=0;i<num;i++) {
        long ran = random() % size;
        samp[i] = data[ran];
    }
    
    floats* res = make_floats(P + 1);
    floats_push(res, 0);
    for(int i=0;i<num;i++) {
        if(i % 3 == 1) {
            floats_push(res, samp[i]);
        }
    }
    floats_push(res, __FLT_MAX__);
    return res;
}

// sum in [0, n]
long sum(long* data, int n) {
    long res = 0;
    for(int i=0;i<=n; ++i) {
        res += data[i];
    }
    return res;
}

void
sort_worker(int pnum, float* data, long size, int P, floats* samps, long* sizes, barrier* bb)
{
    floats* xs = make_floats(10);
    float* lower = samps->data + pnum;
    float* upper = samps->data + pnum + 1;

    for(long i = 0; i < size; ++i) {
        if(compar(&data[i], lower) >= 0 && compar(&data[i], upper) < 0) {
            floats_push(xs, data[i]);
        }
    }
    // TODO: select the floats to be sorted by this worker
    printf("%d: start %.04f, count %ld\n", pnum, samps->data[pnum], xs->size);
    // TODO: some other stuff
    sizes[pnum] = xs->size;

    // puts("floats:");
    // floats_print(xs);
    qsort_floats(xs);

    barrier_wait(bb);
    // TODO: probably more stuff
    long start = sum(sizes, pnum - 1);

    for(long i = start; i < start + xs->size; ++i) {
        data[i] = xs->data[i-start];
    }
    free_floats(xs);
}

void
run_sort_workers(float* data, long size, int P, floats* samps, long* sizes, barrier* bb)
{
    pid_t kids[P];
    (void) kids; // suppress unused warning

    // TODO: spawn P processes, each running sort_worker
    for(int i = 0;i < P; ++i) {
        if((kids[i] = fork())) {
        }
        else {
            sort_worker(i, data, size, P, samps, sizes, bb);
            exit(0);
        }
    }

    for (int ii = 0; ii < P; ++ii) {
        int rv = waitpid(kids[ii], 0, 0);
        check_rv(rv);
    }
}

void
sample_sort(float* data, long size, int P, long* sizes, barrier* bb)
{
    floats* samps = sample(data, size, P);
    qsort_floats(samps);

    // floats_print(samps);
    run_sort_workers(data, size, P, samps, sizes, bb);
    free_floats(samps);
}

int
main(int argc, char* argv[])
{
    alarm(120);

    if (argc != 3) {
        printf("Usage:\n");
        printf("\t%s P data.dat\n", argv[0]);
        return 1;
    }

    const int P = atoi(argv[1]);
    const char* fname = argv[2];

    seed_rng();

    int rv;
    struct stat st;
    rv = stat(fname, &st);
    check_rv(rv);

    const int fsize = st.st_size;
    if (fsize < 8) {
        printf("File too small.\n");
        return 1;
    }

    int fd = open(fname, O_RDWR);
    check_rv(fd);

    // void* file = malloc(1024); // TODO: load the file with mmap.
    void* tmp = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    (void) tmp; // suppress unused warning.
    long count = *((long*)tmp);
    munmap(tmp, 4096);

    void* file = mmap(0, count * sizeof(float), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    (void) file;
    // TODO: These should probably be from the input file.
    float* data = (float*)(file + sizeof(long));

    long sizes_bytes = P * sizeof(long);
    long* sizes = mmap(0, sizes_bytes, PROT_READ | PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0);

    barrier* bb = make_barrier(P);

    sample_sort(data, count, P, sizes, bb);
    // for(long i=0;i<count; i++) {
    //     printf("%f\n", data[i]);
    // }
    free_barrier(bb);

    // TODO: munmap your mmaps
    munmap(file, count * sizeof(float));
    munmap(sizes, sizes_bytes);

    return 0;
}


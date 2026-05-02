#define Z_CONTAINERS_IMPLEMENTATION
#include "z-containers.h"
#include <stdio.h>

typedef struct {
    int id;
    char name[32];
} Person;

int main(void) {
    z_vector v = z_vector_create(sizeof(Person));
    Person p1 = {.id = 1, .name = "Alice"};
    Person p2 = {.id = 2, .name = "Bob"};
    z_vector_push(&v, &p1);
    z_vector_push(&v, &p2);

    Person *p = z_vector_get(&v, 0);
    printf("v[0]: id=%d, name=%s\n", p->id, p->name);

    z_map *m = z_map_create(sizeof(int), sizeof(Person));
    int k = 1;
    z_map_set(m, &k, &p1);

    int *getk = &k;
    Person *getp = z_map_get(m, getk);
    if (getp) printf("map[1]: id=%d, name=%s\n", getp->id, getp->name);

    z_vector_free(&v);
    z_map_free(m);
    return 0;
}
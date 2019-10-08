#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#define TAM_INICIAL 7
#define FACTOR_CARGA_SUPERIOR 0.7
#define FACTOR_CARGA_INFERIOR 0.3
#define FACTOR_REDIMENSION 2


typedef void (*hash_destruir_dato_t)(void *);



static unsigned long hashing_sdbm(const char *str){
  unsigned long hash = 0;
  int c;
  while ((c = *str++)){
    hash = c + (hash << 6) + (hash << 16) - hash;
  }
  return hash;
}


///ESTADOS: 0 = vacio,1 = ocupado, -1 = borrado///
typedef struct campo{
    char* clave;
    void* valor;
    int estado;
}campo_t;


typedef struct hash{
    size_t capacidad;
    size_t cantidad;
    size_t borrados;
    campo_t* campo;
    void (*destruir_dato)(void*);
}hash_t;


bool hash_guardar(hash_t *hash, const char *clave, void *dato);

void redimensionar_hash(hash_t* hash,size_t tamano){
    campo_t* campo_viejo = hash->campo;
    size_t capacidad_vieja = hash->capacidad;
    campo_t* campo_redimensionado = malloc((tamano * sizeof(campo_t)));
    if (campo_redimensionado != NULL){
        hash->capacidad = tamano;
        hash->cantidad = 0;
        hash->borrados = 0;
        hash->campo = campo_redimensionado;

        for(int j = 0;j < hash->capacidad;j++){
            hash->campo[j].clave = NULL;
            hash->campo[j].valor = NULL;
            hash->campo[j].estado = 0;
        }

        for (int i = 0; i<capacidad_vieja; i++){
            if (campo_viejo[i].estado == 1){
                hash_guardar(hash,campo_viejo[i].clave,campo_viejo[i].valor);
            }
        }
        ///// liberar memoria de campo_viejo
    }
}


hash_t *hash_crear(hash_destruir_dato_t destruir_dato){
    hash_t* nuevo_hash = malloc(sizeof(hash_t));
    if (nuevo_hash == NULL) {
        return NULL;
    }
    nuevo_hash->campo = malloc(TAM_INICIAL * sizeof(campo_t));
    if (nuevo_hash->campo == NULL){
        free(nuevo_hash);
        return NULL;
    }

    for(int i = 0;i < TAM_INICIAL;i++){
        nuevo_hash->campo[i].clave = NULL;
        nuevo_hash->campo[i].valor = NULL;
        nuevo_hash->campo[i].estado = 0;
    }

    nuevo_hash->capacidad = TAM_INICIAL;
    nuevo_hash->cantidad = 0;
    nuevo_hash->borrados = 0;
    nuevo_hash->destruir_dato = destruir_dato;

    return nuevo_hash;
}


bool hash_guardar(hash_t *hash, const char *clave, void *dato){
    unsigned long posicion = hashing_sdbm(clave)%(hash->capacidad);


    if ((hash->cantidad + hash->borrados)/hash->capacidad >= FACTOR_CARGA_SUPERIOR){
        redimensionar_hash(hash,hash->capacidad * FACTOR_REDIMENSION);
    }
    return true;
}


bool hash_pertenece_interno(unsigned long key,const hash_t* hash,const char* clave){
    while((hash->campo[key].estado != 0) && (strcmp(hash->campo[key].clave, clave) != 0)){
        key += 1;
        if (key >= hash->capacidad){
            key = 0;
        }
    }
    if((hash->campo[key].estado == 1) && (strcmp(hash->campo[key].clave, clave) == 0)){
        return true;
    }
    return false;
}


bool hash_pertenece(const hash_t *hash, const char *clave){
    unsigned long posicion = hashing_sdbm(clave)%(hash->capacidad);
    return hash_pertenece_interno(posicion,hash,clave);
}


void *hash_obtener(const hash_t *hash, const char *clave){
    unsigned long posicion = hashing_sdbm(clave)%(hash->capacidad);
    if (!hash_pertenece_interno(posicion,hash,clave)){
        return NULL;
    }
    return(hash->campo[posicion].valor);
}


size_t hash_cantidad(const hash_t *hash){
    return (hash->cantidad);
}


void *hash_borrar(hash_t *hash, const char *clave){
    unsigned long posicion = hashing_sdbm(clave)%(hash->capacidad);
    if (!hash_pertenece_interno(posicion,hash,clave)){
        return NULL;
    }
    void* a_devolver = hash->campo[posicion].valor;
    hash->campo[posicion].estado = -1;
    hash->cantidad -= 1;
    hash->borrados += 1;

    if ((hash->cantidad + hash->borrados)/hash->capacidad >= FACTOR_CARGA_INFERIOR){
        redimensionar_hash(hash,hash->capacidad / FACTOR_REDIMENSION);
        /////////////////////////DEBUG///////////////////
        printf("DEBUG REDIMENSION: borrar\n");
        /////////////////////////DEUBG//////////////////////
    }

    return a_devolver;
}


void hash_destruir(hash_t *hash){
    for (int i = 0; i < hash->capacidad;i++){
        if(hash->campo[i].estado != 0){
            if (hash->destruir_dato != NULL){
                hash->destruir_dato(hash->campo[i].clave);
                hash->destruir_dato(hash->campo[i].valor);
            }
            //free(hash->campo[i]);
        }
    }free(hash->campo);
    free(hash);
}

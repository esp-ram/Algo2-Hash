#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#define TAM_INICIAL 101
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


///ESTADOS: 0 = vacio,1 = ocupado, -1 = borrado, 2 = sobreescritura///

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


void inicializa_campos(hash_t* hash){
    for(int i = 0;i < hash->capacidad; i++){
        hash->campo[i].clave = NULL;
        hash->campo[i].valor = NULL;
        hash->campo[i].estado = 0;
    }
}


void redimensionar_hash(hash_t* hash,size_t tamano){
    campo_t* campo_viejo = hash->campo;
    size_t capacidad_vieja = hash->capacidad;
    campo_t* campo_redimensionado = malloc((tamano * sizeof(campo_t)));
    if (campo_redimensionado != NULL){
        hash->capacidad = tamano;
        hash->cantidad = 0;
        hash->borrados = 0;
        hash->campo = campo_redimensionado;
        inicializa_campos(hash);


        for (int i = 0; i<capacidad_vieja; i++){
            if(campo_viejo[i].estado != 0){
                if(campo_viejo[i].estado == 1){
                    hash_guardar(hash,campo_viejo[i].clave,campo_viejo[i].valor);
                }
            }
            free(campo_viejo[i].clave);
        }
        free(campo_viejo);
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

    nuevo_hash->capacidad = TAM_INICIAL;
    nuevo_hash->cantidad = 0;
    nuevo_hash->borrados = 0;
    nuevo_hash->destruir_dato = destruir_dato;
    inicializa_campos(nuevo_hash);

    return nuevo_hash;
}


bool hash_guardar(hash_t *hash, const char *clave, void *dato){
    unsigned long posicion = hashing_sdbm(clave)%(hash->capacidad);

    while((hash->campo[posicion].estado != 0) && (hash->campo[posicion].estado != 2)){
        if ((hash->campo[posicion].estado == 1)&&(strcmp(hash->campo[posicion].clave, clave) == 0)){
            void* dato_a_destruir = hash->campo[posicion].valor;
            if (hash->destruir_dato != NULL){
                hash->destruir_dato(dato_a_destruir);
            }
            hash->campo[posicion].valor = NULL;
            hash->campo[posicion].estado = 2;
        }else{
            posicion += 1;
            if (posicion >= hash->capacidad){
                 posicion = 0;
             }
         }
    }

    if (hash->campo[posicion].estado == 0){
        hash->campo[posicion].clave = calloc(strlen(clave)+1,sizeof(char));
        if (hash->campo[posicion].clave == NULL){
            return false;
        }
        strcpy(hash->campo[posicion].clave, clave);
        hash->cantidad += 1;
    }
    hash->campo[posicion].valor = dato;
    hash->campo[posicion].estado = 1;

    if (((float)hash->cantidad + (float)hash->borrados) / (float)hash->capacidad >= FACTOR_CARGA_SUPERIOR){
        redimensionar_hash(hash,hash->capacidad * FACTOR_REDIMENSION);
    }
    return true;
}


unsigned long ubicacion_correcta(const hash_t *hash, const char *clave){
    unsigned long posicion = hashing_sdbm(clave)%(hash->capacidad);
    while((hash->campo[posicion].estado != 0) && (strcmp(hash->campo[posicion].clave, clave) != 0)){
        posicion += 1;
        if (posicion >= hash->capacidad){
            posicion = 0;
        }
    }
    return posicion;
}


bool hash_pertenece(const hash_t *hash, const char *clave){
    unsigned long posicion = ubicacion_correcta(hash,clave);
    if((hash->campo[posicion].estado == 1) && strcmp(hash->campo[posicion].clave, clave) == 0){
        return true;
    }
    return false;
}


void *hash_obtener(const hash_t *hash, const char *clave){
    unsigned long posicion = ubicacion_correcta(hash,clave);
    if((hash->campo[posicion].estado == 1) && strcmp(hash->campo[posicion].clave, clave) == 0){
        return hash->campo[posicion].valor;
    }
    return NULL;
}


size_t hash_cantidad(const hash_t *hash){
    return (hash->cantidad);
}


void *hash_borrar(hash_t *hash, const char *clave){
    unsigned long posicion;
    if (hash_pertenece(hash,clave)){
        posicion = ubicacion_correcta(hash,clave);
    }else{
        return NULL;
    }
    void* a_devolver = hash->campo[posicion].valor;
    hash->campo[posicion].estado = -1;
    hash->cantidad -= 1;
    hash->borrados += 1;

    if (((float)hash->cantidad / (float)hash->capacidad) <= FACTOR_CARGA_INFERIOR && ((hash->capacidad) > TAM_INICIAL)){
        redimensionar_hash(hash,hash->capacidad / FACTOR_REDIMENSION);
    }
    return a_devolver;
}


void hash_destruir(hash_t *hash){
    for (int i = 0; i < hash->capacidad;i++){
        if(hash->campo[i].estado != 0){
            if (hash->destruir_dato != NULL){
                hash->destruir_dato(hash->campo[i].valor);
            }
            free(hash->campo[i].clave);
        }
    }free(hash->campo);
    free(hash);
}





typedef struct hash_iter hash_iter_t;

struct hash_iter{
    const hash_t* hash;
    size_t posicion;
};
//--------------------------------------------------------------------

void avanza_a_ocupado(hash_iter_t* iter){
    while((iter->posicion < iter->hash->capacidad) && iter->hash->campo[iter->posicion].estado != 1){
            iter->posicion += 1;
    }
}


// Crea iterador
hash_iter_t* hash_iter_crear(const hash_t* hash){
    if(!hash) return NULL;
    hash_iter_t* iter = malloc(sizeof(hash_iter_t));
    if(!iter) return NULL;
    iter->hash = hash;
    iter->posicion = 0;
    avanza_a_ocupado(iter);
    return iter;
}
//---------------------------------------------------

// Comprueba si terminó la iteración
bool hash_iter_al_final(const hash_iter_t* iter){
    return ((size_t)iter->posicion == (size_t)iter->hash->capacidad);
}

//------------------------------------------------------

// Avanza iterador
bool hash_iter_avanzar(hash_iter_t* iter){
    if(hash_iter_al_final(iter)) return false;
    iter->posicion += 1;
    avanza_a_ocupado(iter);
    return true;
}
//---------------------------------------------------

// Devuelve clave actual, esa clave no se puede modificar ni liberar.
char* hash_iter_ver_actual(const hash_iter_t* iter){
    if(hash_iter_al_final(iter) || iter->hash->campo[iter->posicion].estado != 1) return NULL;
    return (char*)iter->hash->campo[iter->posicion].clave;
}
//---------------------------------------------------


// Destruye iterador
void hash_iter_destruir(hash_iter_t* iter){
    free(iter);
}

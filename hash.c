// HASH CERRADO -- Corrector: Diego Balestieri - 2do cuat 2019
// Esperon Ramiro - 103992
// dos Santos Claro Ignacio - 102769

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#define CAPACIDAD_INICIAL 101
#define FACTOR_DE_CARGA_MAX 0.7
#define FACTOR_DE_CARGA_MIN 0.3
#define FACTOR_REDIMENSION 2

typedef void (*hash_destruir_dato_t)(void* );

//---------------------FUNCION DE HASHING----------------------------------------//

static size_t  funcion_de_hashing(const char* str){
  size_t  hash = 0;
  int c;
  while ((c =* str++)){
    hash = c + (hash << 6) + (hash << 16) - hash;
  }
  return hash;
}

//-------------------------------------------------------------------------------//

///ESTADOS: 0 = vacio,1 = ocupado, -1 = borrado, 2 = sobreescritura///

typedef struct campo campo_t;
struct campo{
    char* clave;
    void* valor;
    int estado;
};

typedef struct hash{
    size_t capacidad;
    size_t cantidad;
    size_t borrados;
    campo_t** campo;
    void (*destruir_dato)(void*);
}hash_t;

bool hash_guardar(hash_t* hash, const char* clave, void* dato);

/////////////////////////////////////////////////////////ADDED TAG
void campo_crear(hash_t* hash){
    for(int i = 0;i < hash->capacidad; i++){
        hash->campo[i]->clave = NULL;
        hash->campo[i]->valor = NULL;
        hash->campo[i]->estado = 0;
    }
}

/////////////////////////////////////////////////////////ADDED TAG
void redimensionar(hash_t* hash,size_t tamano){
    campo_t** campo_viejo = hash->campo;
    size_t capacidad_vieja = hash->capacidad;
    campo_t** campo_redimensionado = malloc((tamano*sizeof(campo_t*)));
    if (campo_redimensionado != NULL){
        hash->capacidad = tamano;
        hash->cantidad = 0;
        hash->borrados = 0;
        hash->campo = campo_redimensionado;
        campo_crear(hash);
        for (int i = 0; i<capacidad_vieja; i++){
            if(campo_viejo[i]->estado != 0){
                if(campo_viejo[i]->estado == 1){
                    hash_guardar(hash,campo_viejo[i]->clave,campo_viejo[i]->valor);
                }
            }
            free(campo_viejo[i]->clave);
        }
        free(campo_viejo);
    }
}

/*** ****************************************************************
*                       PRIMITIVAS DEL HASH CERRADO
** * ***************************************************************/

hash_t* hash_crear(hash_destruir_dato_t destruir_dato){
    hash_t* nuevo_hash = malloc(sizeof(hash_t));
    if (nuevo_hash == NULL) {
        return NULL;
    }
    nuevo_hash->campo = malloc(CAPACIDAD_INICIAL*sizeof(campo_t*));
    if (nuevo_hash->campo == NULL){
        free(nuevo_hash);
        return NULL;
    }
    nuevo_hash->capacidad = CAPACIDAD_INICIAL;
    nuevo_hash->cantidad = 0;
    nuevo_hash->borrados = 0;
    nuevo_hash->destruir_dato = destruir_dato;
    campo_crear(nuevo_hash);
    return nuevo_hash;
}

bool hash_guardar(hash_t* hash, const char* clave, void* dato){
    size_t  posicion = funcion_de_hashing(clave)%(hash->capacidad);
    while((hash->campo[posicion]->estado != 0) && (hash->campo[posicion]->estado != 2)){
        if ((hash->campo[posicion]->estado == 1)&&(strcmp(hash->campo[posicion]->clave, clave) == 0)){
            void* dato_a_destruir = hash->campo[posicion]->valor;
            if (hash->destruir_dato != NULL){
                hash->destruir_dato(dato_a_destruir);
            }
            hash->campo[posicion]->valor = NULL;
            hash->campo[posicion]->estado = 2;
        }else{
            posicion += 1;
            if (posicion >= hash->capacidad){
                 posicion = 0;
            }
        }
    }
    if (hash->campo[posicion]->estado == 0){
        hash->campo[posicion]->clave = calloc(strlen(clave)+1,sizeof(char));
        if (hash->campo[posicion]->clave == NULL){
            return false;
        }
        strcpy(hash->campo[posicion]->clave, clave);
        hash->cantidad += 1;
    }
    hash->campo[posicion]->valor = dato;
    hash->campo[posicion]->estado = 1;
    if (((float)hash->cantidad + (float)hash->borrados) / (float)hash->capacidad >= FACTOR_DE_CARGA_MAX){
        redimensionar(hash,hash->capacidad*  FACTOR_REDIMENSION);
    }
    return true;
}

/////////////////////////////////////////////////////////ADDED TAG
size_t  ubicacion_correcta(const hash_t* hash, const char* clave){
    size_t  posicion = funcion_de_hashing(clave)%(hash->capacidad);
    while(hash->campo[posicion]->estado != 0){
        if ((hash->campo[posicion]->estado == 1) && (strcmp(hash->campo[posicion]->clave, clave) == 0)){
            return posicion;
        }else{
            posicion += 1;
            if (posicion >= hash->capacidad){
                posicion = 0;
            }
        }
    }
    return posicion;
}

bool hash_pertenece(const hash_t* hash, const char* clave){
    size_t  posicion = funcion_de_hashing(clave)%(hash->capacidad);
    while(hash->campo[posicion]->estado != 0){
        if ((hash->campo[posicion]->estado == 1) && (strcmp(hash->campo[posicion]->clave, clave) == 0)){
            return true;
        }else{
            posicion += 1;
            if (posicion >= hash->capacidad){
                posicion = 0;
            }
        }
    }
    return false;
}

void* hash_obtener(const hash_t* hash, const char* clave){
    if (hash_pertenece(hash,clave)){
        size_t  posicion = ubicacion_correcta(hash,clave);
        return hash->campo[posicion]->valor;
    }
    return NULL;
}

size_t hash_cantidad(const hash_t* hash){
    return (hash->cantidad);
}

void* hash_borrar(hash_t* hash, const char* clave){
    size_t  posicion;
    if (hash_pertenece(hash,clave)){
        posicion = ubicacion_correcta(hash,clave);
    }else{
        return NULL;
    }
    void* a_devolver = hash->campo[posicion]->valor;
    hash->campo[posicion]->estado = -1;
    hash->cantidad -= 1;
    hash->borrados += 1;
    if (((float)hash->cantidad / (float)hash->capacidad) <= FACTOR_DE_CARGA_MIN && ((hash->capacidad) > CAPACIDAD_INICIAL)){
        redimensionar(hash,hash->capacidad / FACTOR_REDIMENSION);
    }
    return a_devolver;
}

void hash_destruir(hash_t* hash){
    for (int i = 0; i < hash->capacidad;i++){
        if(hash->campo[i]->estado != 0){
            if (hash->destruir_dato != NULL){
                hash->destruir_dato(hash->campo[i]->valor);
            }
            free(hash->campo[i]->clave);
        }
    }free(hash->campo);
    free(hash);
}

/*** ****************************************************************
*                         PRIMITIVAS DEL ITERADOR
** * ***************************************************************/

//---------definiion de la estructura del iterador del hash-----------
typedef struct hash_iter hash_iter_t;
struct hash_iter{
    const hash_t* hash;
    campo_t* actual;
    size_t posicion;
};
//--------------------------------------------------------------------

// Crea iterador
hash_iter_t* hash_iter_crear(const hash_t* hash){
    if(!hash) return NULL;
    hash_iter_t* iter = malloc(sizeof(hash_iter_t));
    if(!iter) return NULL;
    iter->hash = hash;
    iter->posicion = 0;
    iter->actual = hash->campo[iter->posicion];
    return iter;
}
//---------------------------------------------------

// Comprueba si terminó la iteración
bool hash_iter_al_final(const hash_iter_t* iter){
    return iter->posicion == iter->hash->capacidad-1;
}
//------------------------------------------------------

// Avanza iterador
bool hash_iter_avanzar(hash_iter_t* iter){
    if (!iter || hash_iter_al_final(iter)) return false;
    if (iter->posicion < iter->hash->capacidad){
        iter->posicion++;
        iter->actual = iter->hash->campo[iter->posicion];
        return true;
    }
}
//---------------------------------------------------

// Devuelve clave actual, esa clave no se puede modificar ni liberar.
char* hash_iter_ver_actual(const hash_iter_t* iter){
    if(!iter) return NULL;
    return (char*)iter->hash->campo[iter->posicion]->clave;
}
//---------------------------------------------------


// Destruye iterador
void hash_iter_destruir(hash_iter_t* iter){
    free(iter);
}
//------------------------------------------------------
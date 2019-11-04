// TABLA DE HASH (CERRADO)-- Corrector: Diego Balestieri - 2do cuat 2019
// Grupo Nº45
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

typedef enum {VACIO, OCUPADO, BORRADO} estado_t;

typedef struct campo{
    char* clave;
    void* valor;
    estado_t estado;
}campo_t;


typedef struct hash{
    size_t capacidad;
    size_t cantidad;
    size_t borrados;
    campo_t* campo;
    void (*destruir_dato)(void*);
}hash_t;

//-------------------------------FUNCION DE HASHING------------------------------//

static size_t  funcion_de_hashing(const char* str, size_t modulo){
  size_t  indice = 0;
  int c;
  while ((c =* str++)){
    indice = c + (indice << 6) + (indice << 16) - indice;
  }
  return indice%modulo;
}

//-------------------------------------------------------------------------------//
bool hash_guardar(hash_t* hash, const char* clave, void* dato);

/////////////////////////////////////////////////////////FUNCION AUXILIAR
void inicializar_campos(hash_t* hash){
    for(int i = 0;i < hash->capacidad; i++){
        hash->campo[i].clave = NULL;
        hash->campo[i].valor = NULL;
        hash->campo[i].estado = VACIO;
    }
}

/////////////////////////////////////////////////////////FUNCION AUXILIAR
void redimensionar(hash_t* hash,size_t tamano){
    campo_t* campo_viejo = hash->campo;
    size_t capacidad_vieja = hash->capacidad;
    campo_t* campo_redimensionado = malloc((tamano*  sizeof(campo_t)));
    if (campo_redimensionado != NULL){
        hash->capacidad = tamano;
        hash->cantidad = 0;
        hash->borrados = 0;
        hash->campo = campo_redimensionado;
        inicializar_campos(hash);
        for (int i = 0; i<capacidad_vieja; i++){
            if(campo_viejo[i].estado != VACIO){
                if(campo_viejo[i].estado == OCUPADO){
                    hash_guardar(hash,campo_viejo[i].clave,campo_viejo[i].valor);
                }
            }
            free(campo_viejo[i].clave);
        }
        free(campo_viejo);
    }
}
/********************************************************************
*                       PRIMITIVAS DEL HASH CERRADO                 *
*********************************************************************/
/////////////////////////////////////////////////////////FUNCION AUXILIAR
size_t  ubicacion_correcta(const hash_t* hash, const char* clave){
    size_t  posicion = funcion_de_hashing(clave, hash->capacidad);
    while((hash->campo[posicion].estado != VACIO) && (strcmp(hash->campo[posicion].clave, clave) != 0)){
        posicion++;
        if (posicion >= hash->capacidad){
           posicion = 0;
        }
    }
    return posicion;
}

hash_t* hash_crear(hash_destruir_dato_t destruir_dato){
    hash_t* nuevo_hash = malloc(sizeof(hash_t));
    if (nuevo_hash == NULL) {
        return NULL;
    }
    nuevo_hash->campo = malloc(CAPACIDAD_INICIAL*  sizeof(campo_t));
    if (nuevo_hash->campo == NULL){
        free(nuevo_hash);
        return NULL;
    }
    nuevo_hash->capacidad = CAPACIDAD_INICIAL;
    nuevo_hash->cantidad = 0;
    nuevo_hash->borrados = 0;
    nuevo_hash->destruir_dato = destruir_dato;
    inicializar_campos(nuevo_hash);
    return nuevo_hash;
}

bool hash_guardar(hash_t* hash, const char* clave, void* dato){
    size_t posicion = ubicacion_correcta(hash, clave);
    if (hash->campo[posicion].estado == OCUPADO && strcmp(hash->campo[posicion].clave, clave) == 0){			// si ya existe la clave, le pisamos su valor actual con el nuevo
    	if (hash->destruir_dato != NULL){																		// pero primero borramos el que ya no vamos a considerar
    		hash->destruir_dato(hash->campo[posicion].valor);
    	}
    }
    if (hash->campo[posicion].estado == VACIO){
        hash->campo[posicion].clave = calloc(strlen(clave)+1,sizeof(char));
        if (hash->campo[posicion].clave == NULL){
            return false;
        }
        strcpy(hash->campo[posicion].clave, clave);
        hash->cantidad++;
    	hash->campo[posicion].estado = OCUPADO;
    }
    hash->campo[posicion].valor = dato;
    if (((float)hash->cantidad + (float)hash->borrados) / (float)hash->capacidad >= FACTOR_DE_CARGA_MAX){
        redimensionar(hash,hash->capacidad*  FACTOR_REDIMENSION);
    }
    return true;
}


bool hash_pertenece(const hash_t* hash, const char* clave){
    size_t  posicion = ubicacion_correcta(hash, clave);
    if ((hash->campo[posicion].estado == OCUPADO) && (strcmp(hash->campo[posicion].clave, clave) == 0)){
        return true;
    }
    return false;
}

void* hash_obtener(const hash_t* hash, const char* clave){
 	size_t  posicion = ubicacion_correcta(hash,clave);
	if(hash->campo[posicion].estado == OCUPADO && strcmp(hash->campo[posicion].clave, clave) == 0){
		return hash->campo[posicion].valor;
	}
    return NULL;
}

size_t hash_cantidad(const hash_t* hash){
    return (hash->cantidad);
}

void* hash_borrar(hash_t* hash, const char* clave){
    size_t posicion = ubicacion_correcta(hash, clave);
    void* a_devolver = hash->campo[posicion].valor;			//agarramos la clave y, si pertenece, retenemos su posicion y guardamos su dato en una variable
    if (hash->destruir_dato != NULL){						//borramos el dato y la clave
            free(hash->campo[posicion].clave);
            hash->destruir_dato(hash->campo[posicion].valor);
    }
    else{
        return NULL;
    }
    hash->campo[posicion].estado = BORRADO;
    hash->cantidad--;
    hash->borrados++;
    if (((float)hash->cantidad / (float)hash->capacidad) <= FACTOR_DE_CARGA_MIN && ((hash->capacidad) > CAPACIDAD_INICIAL)){
        redimensionar(hash,hash->capacidad / FACTOR_REDIMENSION);
    }
    return a_devolver;
}

void hash_destruir(hash_t* hash){
    for (int i = 0; i < hash->capacidad; i++){
        if(hash->campo[i].estado != VACIO){
            if (hash->destruir_dato != NULL){
                hash->destruir_dato(hash->campo[i].valor);
            }
            free(hash->campo[i].clave);
        }
    }free(hash->campo);
    free(hash);
}

/**** ***************************************************************
*                         PRIMITIVAS DEL ITERADOR
*** **************************************************************/

//---------definiion de la estructura del iterador del hash-----------
typedef struct hash_iter hash_iter_t;
struct hash_iter{
    const hash_t* hash;
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
    while((iter->posicion < iter->hash->capacidad) && iter->hash->campo[iter->posicion].estado != OCUPADO){
            iter->posicion++;
    }
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
    if (hash_iter_al_final(iter)) return false;
    iter->posicion++;
    while((iter->posicion < iter->hash->capacidad) && iter->hash->campo[iter->posicion].estado != OCUPADO){
            iter->posicion++;
    }
    return true;
}
//---------------------------------------------------

// Devuelve clave actual, esa clave no se puede modificar ni liberar.
const char* hash_iter_ver_actual(const hash_iter_t* iter){
    if(hash_iter_al_final(iter) || iter->hash->campo[iter->posicion].estado == BORRADO) return NULL;
   	return (const char*)iter->hash->campo[iter->posicion].clave;
}
//---------------------------------------------------


// Destruye iterador
void hash_iter_destruir(hash_iter_t* iter){
    free(iter);
}
//------------------------------------------------------
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "hash.h"
#define TAM_INICIAL 32
#define FACTOR_CARGA 0.6
#define FACTOR_REDIMENSION 2


typedef void (*hash_destruir_dato_t)(void *);

//---------------------FUNCION DE HASHING----------------------------------------//

static unsigned long hashing_sdbm(const char *str){
  unsigned long hash = 0;
  int c;
  while ((c = *str++)){
    hash = c + (hash << 6) + (hash << 16) - hash;
  }
  return hash;
}

//-------------------------------------------------------------------------------//

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
    campo_t** campo;
    void (*destruir_dato)(void*);
}hash_t;


campo_t* new_campo(){
    campo_t* nuevo_campo = malloc(sizeof(campo_t));
    nuevo_campo->clave = NULL;
    nuevo_campo->valor = NULL;
    nuevo_campo->estado = 0;
    return nuevo_campo;
}


/////////////////////////////////////////////////////////ADDED TAG
void redimensionar_hash(hash_t* hash,size_t tamano){
    campo_t** campo_viejo = hash->campo;
    size_t capacidad_vieja = hash->capacidad;
    campo_t** campo_redimensionado = malloc((tamano * sizeof(campo_t*)));
    if (campo_redimensionado != NULL){
        hash->capacidad = tamano;
        hash->cantidad = 0;
        hash->borrados = 0;
        hash->campo = campo_redimensionado;

        for(int j = 0;j < hash->capacidad;j++){
          hash->campo[j] = new_campo();
        }

        for(int i = 0; i < capacidad_vieja; i++){
            if (campo_viejo[i]->estado == 1){
              /////////////////ERROR/////////////////ERROR/////////////////ERROR/////////////////ERROR
              //////TEST/////
              char* clave_n = calloc(strlen(campo_viejo[i]->clave+1),sizeof(char));
              strcpy(clave_n, campo_viejo[i]->clave);
              //////TEST/////
              //char* clave_n = campo_viejo[i]->clave;
              void* valor_n = campo_viejo[i]->valor;
              hash_guardar(hash,campo_viejo[i]->clave,campo_viejo[i]->valor);
            }
        }
        ///// liberar memoria de campo_viejo
    }
}


//-------------------------------------------------------------------------------------------------------------------------
//                                              PRIMITIVAS DEL HASH
//-------------------------------------------------------------------------------------------------------------------------
//Recibe una funcion destruir para el tipo de dato que se vaya a guardar en el hash
//Crea la estructura hash devolviendo un puntero a ella
//Pre: La funcion destruir puede ser NULL en caso de no ser necesaria
//Post: Se instanciaron los atributos de la estructura y se esta fue creada
hash_t *hash_crear(hash_destruir_dato_t destruir_dato){
    hash_t* nuevo_hash = malloc(sizeof(hash_t));
    if (nuevo_hash == NULL) {
        return NULL;
    }
    nuevo_hash->campo = malloc(TAM_INICIAL * sizeof(campo_t*));
    if (nuevo_hash->campo == NULL){
        free(nuevo_hash);
        return NULL;
    }

    for(int i = 0;i< TAM_INICIAL;i++){
        nuevo_hash->campo[i] = new_campo();
    }

    nuevo_hash->capacidad = TAM_INICIAL;
    nuevo_hash->cantidad = 0;
    nuevo_hash->borrados = 0;
    nuevo_hash->destruir_dato = destruir_dato;

    return nuevo_hash;
}


//Recibe un puntero a un hash, una clave del hash y cualquier tipo de dato a guardar
//Guarda el dato en su respectiva clave dentro del hash
//Devuelve true en caso de exito, o false en caso de no haber podido guardar el dato
//Pre: Si la clave no existia se crea una y se guarda el dato, el puntero al hash no es Nulo
//Post: Se guardo el dato en la clave pasada por paremtro, dentro del hash
bool hash_guardar(hash_t *hash, const char *clave, void *dato){
    unsigned long posicion = hashing_sdbm(clave)%(hash->capacidad);
    //REEMPLAZO
    if ((hash->campo[posicion]->estado == 1)&&(strcmp(hash->campo[posicion]->clave, clave) == 0)){
        void* dato_a_destruir = hash->campo[posicion]->valor;
        if (hash->destruir_dato != NULL){
            hash->destruir_dato(dato_a_destruir);
        }
    }else {
        while(hash->campo[posicion]->estado != 0){
            posicion ++;
            if (posicion >= hash->capacidad){
                posicion = 0;
            }
        }
        hash->campo[posicion]->clave = calloc(strlen(clave)+1,sizeof(char));
        strcpy(hash->campo[posicion]->clave, clave);
        hash->cantidad += 1;
    }
    hash->campo[posicion]->valor = dato;
    hash->campo[posicion]->estado = 1;
    if ((hash->cantidad + hash->borrados)/hash->capacidad >= FACTOR_CARGA){
        redimensionar_hash(hash,hash->capacidad * FACTOR_REDIMENSION);
    }
    return true;
}


void *hash_borrar(hash_t *hash, const char *clave){
    unsigned long posicion = hashing_sdbm(clave)%(hash->capacidad);
    if (!hash_pertenece_interno(posicion,hash,clave)){
        return NULL;
    }
    void* a_devolver = hash->campo[posicion]->valor;
    hash->campo[posicion]->estado = -1;
    hash->cantidad -= 1;
    hash->borrados += 1;

    ////////////REDIMENSIONAR////////////

    return a_devolver;
}


//Recibe un puntero a hash y una clave perteneciente al hash, si no existe devuelve NULL,
//Devuelve el dato contenido en la posicion de la clave del hash
//Pre: El hash fue creado
//Post: Dedevolvio el valor contenido en la clave, o NULL en caso de que la clave no pertenezca al hash
void *hash_obtener(const hash_t *hash, const char *clave){
    unsigned long posicion = hashing_sdbm(clave)%(hash->capacidad);
    if (!hash_pertenece_interno(posicion,hash,clave)){
        return NULL;
    }
    return (hash->campo[posicion]->valor);
}


//Recibe un puntero a hash y una clave, se fija si esta pertenece al hash y devuelve true en dicho caso
//o false en caso contrario
//Pre: El hash fue creado
//Post: Se devolvio true o false en caso de pertenecer o no, la clave, al hash
bool hash_pertenece(const hash_t *hash, const char *clave){
    unsigned long posicion = hashing_sdbm(clave)%(hash->capacidad);
    return hash_pertenece_interno(posicion,hash,clave);
}


/////////////////////////////////////////////////////////ADDED TAG
bool hash_pertenece_interno(unsigned long key,const hash_t* hash,const char* clave){
    while((hash->campo[key]->estado != 0) && (strcmp(hash->campo[key]->clave, clave) != 0)){
        key ++;
        if (key >= hash->capacidad){
            key = 0;
        }
    }
    if((hash->campo[key]->estado == 1) && (strcmp(hash->campo[key]->clave, clave) == 0)){
        return true;
    }
    return false;
}


//Recibe un puntero a hash y devuelve la cantidad de claves contenidas
//Pre: El hash fue creado
//Post: Se devolvio la cantidad de claves guardadas en el hash
size_t hash_cantidad(const hash_t *hash){
    return (hash->cantidad);
}


//Recibe un puntero a hash, y lo borra, aplicandole su funcion desdtruir a cada elemento
//Pre: El hash fue creado
//Post: Se liberaron todos los datos contenidos en el hash y el hash mismo a su vez
void hash_destruir(hash_t *hash){
    for (int i = 0; i < hash->capacidad;i++){
        if(hash->campo[i]->estado != 0){
            if (hash->destruir_dato != NULL){
                hash->destruir_dato(hash->campo[i]->clave);
                hash->destruir_dato(hash->campo[i]->valor);
            }
            free(hash->campo[i]);
        }
    }free(hash->campo);
    free(hash);
}

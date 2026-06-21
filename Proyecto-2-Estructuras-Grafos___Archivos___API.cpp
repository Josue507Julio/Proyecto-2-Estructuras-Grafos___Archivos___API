/*
    PROYECTO PROGRAMADO: GRAFOS Y ARCHIVOS
    Sistema de rutas intergalacticas

    Fecha de inicio: 17/06/2026
    Ultima modificacion: 20/06/2026
    Integrantes: 
        Yohan Andrey Morera Ramírez     2025105204
        Jaiden Joel Avila Badilla       2025119253
        Josué David González Alvarado   2024297026

    Descripcion:
    El programa consume una API con libcurl y procesa la respuesta JSON.
    Las galaxias, rutas, naves y viajes se almacenan con estructuras struct,
    listas enlazadas y punteros. El grafo se representa mediante una lista
    de galaxias y una sublista de arcos para cada galaxia.

    Dependencias exigidas por el proyecto:
    - libcurl
    - json.hpp de nlohmann

    Ejecución:
    1. Abrir una nueva terminal
    2. En la ruta del proyecto, ejecutar estos comandos:
        a. g++ -std=c++17 Pro2_2025105204.cpp -o Pro2_2025105204.exe -lcurl
        b. ./Pro2_2025105204.exe
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <limits>
#include <cstdlib>
#include <ctime>
#include <cctype>
#include <curl/curl.h>
#include "json.hpp"

using namespace std;
using json = nlohmann::json;

// Direccion base utilizada para construir los endpoints de la API.
const string URL_BASE = "https://galaxias-mock-api.onrender.com";
// Valor utilizado para representar una distancia que aun no ha sido alcanzada.
const double INFINITO = 999999999.0;

// ============================================================================
// DECLARACION DE ESTRUCTURAS
// ============================================================================

struct Ruta;
struct Galaxia;
struct Nave;

// Arco perteneciente a la lista de adyacencia de una galaxia.
struct Arco {
    Galaxia* destino;
    Ruta* ruta;
    Arco* sigA;
};

// Vertice del grafo.
struct Galaxia {
    string id;
    string codigo;
    string nombre;
    string tipo;
    string descripcion;
    double x;
    double y;
    double z;
    Arco* subListaArcos;
    Galaxia* sigG;
};

// Ruta original entre dos galaxias.
struct Ruta {
    string id;
    Galaxia* origen;
    Galaxia* destino;
    double costo;
    bool dirigida;
    Ruta* sigR;
};

// Nave registrada.
struct Nave {
    string id;
    string codigo;
    string nombre;
    Nave* sigN;
};

// Cada paso enlaza una ruta utilizada durante un viaje.
struct PasoViaje {
    Ruta* ruta;
    string idRuta;
    PasoViaje* siguiente;
};

// Registro de un viaje realizado por una nave.
struct Viaje {
    string id;
    Nave* nave;
    Galaxia* origen;
    Galaxia* destino;
    double costoTotal;
    string fecha;
    PasoViaje* rutasUsadas;
    Viaje* sigV;
};

// Resultado auxiliar para Dijkstra.
struct ResultadoCamino {
    bool encontrado;
    double costoTotal;
    vector<Galaxia*> galaxias;
    vector<Ruta*> rutas;
};

// ============================================================================
// PUNTEROS PRINCIPALES
// ============================================================================

// Punteros que conservan el inicio de cada lista enlazada principal del sistema.
Galaxia* primeraGalaxia = NULL;
Ruta* primeraRuta = NULL;
Nave* primeraNave = NULL;
Viaje* primerViaje = NULL;

// Guarda las rutas recibidas desde /grafo/kruskal en el orden aleatorio.
vector<Ruta*> ordenKruskal;

// Contador utilizado por el reporte estadistico para registrar las consultas de rutas minimas.
int cantidadRutasCalculadas = 0;

// ============================================================================
// PROTOTIPOS
// ============================================================================

Galaxia* buscarGalaxia(string valor);
Galaxia* buscarGalaxiaPorId(string id);
Galaxia* buscarGalaxiaPorCodigo(string codigo);
Galaxia* buscarGalaxiaPorNombre(string nombre);
Ruta* buscarRuta(string id);
Nave* buscarNave(string valor);
Nave* buscarNavePorId(string id);
Nave* buscarNavePorCodigo(string codigo);
Nave* buscarNavePorNombre(string nombre);
Viaje* buscarViaje(string id);
void reconstruirGrafo();
void guardarDatosLocales();
void modificarViaje();
bool galaxiaUsadaEnHistorial(Galaxia* galaxia);
bool existeConexionDuplicada(Galaxia* origen, Galaxia* destino, bool nuevaDirigida, Ruta* ignorar = NULL);
int contarGalaxias();
int contarRutas();
int contarNaves();
int contarViajes();

// ============================================================================
// FUNCIONES DE ENTRADA Y VALIDACION
// ============================================================================


// Restablece el flujo de entrada y elimina los caracteres pendientes para evitar errores entre lecturas.
void limpiarEntrada() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}


// Solicita un numero entero y repite la lectura hasta que el usuario ingrese un valor valido.
int leerEntero(string mensaje) {
    int numero;

    while (true) {
        cout << mensaje;
        cin >> numero;

        if (!cin.fail()) {
            limpiarEntrada();
            return numero;
        }

        cout << "Entrada invalida. Digite un numero entero." << endl;
        limpiarEntrada();
    }
}


// Solicita un numero decimal y valida que la entrada pueda convertirse correctamente.
double leerDecimal(string mensaje) {
    double numero;

    while (true) {
        cout << mensaje;
        cin >> numero;

        if (!cin.fail()) {
            limpiarEntrada();
            return numero;
        }

        cout << "Entrada invalida. Digite un numero." << endl;
        limpiarEntrada();
    }
}


// Muestra un mensaje y obtiene una linea completa de texto desde la entrada estandar.
string leerTexto(string mensaje) {
    string texto;
    cout << mensaje;
    getline(cin, texto);
    return texto;
}


// Comprueba si una cadena no contiene caracteres y retorna verdadero cuando esta vacia.
bool textoVacio(string texto) {
    return texto.length() == 0;
}

// Evita que los separadores dañen el formato de los archivos TXT.
bool textoContieneSeparadores(string texto) {
    return texto.find('|') != string::npos || texto.find(',') != string::npos;
}

// ============================================================================
// CONSUMO DE LA API CON CURL
// ============================================================================

// libcurl llama esta funcion cada vez que recibe una parte de la respuesta.
size_t guardarRespuesta(void* contenido, size_t tamano, size_t cantidad, void* destino) {
    size_t total = tamano * cantidad;
    string* respuesta = (string*)destino;
    respuesta->append((char*)contenido, total);
    return total;
}


// Construye la URL, consulta un endpoint con libcurl y retorna el contenido recibido como texto.
string consumirEndpoint(string endpoint) {
    string url = URL_BASE + endpoint;

    // Se realizan varios intentos porque el servidor puede tardar en responder.
    for (int intento = 1; intento <= 3; intento++) {
        CURL* curl = curl_easy_init();
        string respuesta = "";
        char mensajeError[CURL_ERROR_SIZE] = {0};

        if (curl == NULL) {
            cout << "No se pudo iniciar libcurl." << endl;
            return "";
        }

        struct curl_slist* encabezados = NULL;
        encabezados = curl_slist_append(encabezados, "Accept: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, encabezados);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, guardarRespuesta);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &respuesta);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 20L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 90L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
        curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "ProyectoGrafos/1.0");
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, mensajeError);

        // En Windows se intenta usar el almacen de certificados del sistema.
        // La verificacion SSL permanece activada.
#ifdef CURLSSLOPT_NATIVE_CA
        curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif

        CURLcode resultado = curl_easy_perform(curl);
        long codigoHttp = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &codigoHttp);

        curl_slist_free_all(encabezados);
        curl_easy_cleanup(curl);

        if (resultado == CURLE_OK && codigoHttp >= 200 && codigoHttp < 300 &&
            respuesta != "") {
            return respuesta;
        }

        cout << "Intento " << intento << " de 3 para " << endpoint
             << " no fue exitoso." << endl;

        if (resultado != CURLE_OK) {
            cout << "Detalle CURL: ";
            if (mensajeError[0] != '\0') cout << mensajeError;
            else cout << curl_easy_strerror(resultado);
            cout << " (codigo " << (int)resultado << ")" << endl;
        } else {
            cout << "Respuesta HTTP: " << codigoHttp << endl;
            if (respuesta != "") {
                cout << "Inicio de la respuesta: "
                     << respuesta.substr(0, 200) << endl;
            }
        }
    }

    return "";
}


// Consume un endpoint, analiza su respuesta y retorna un objeto JSON valido o un valor vacio.
json obtenerJson(string endpoint) {
    string texto = consumirEndpoint(endpoint);

    if (texto == "") {
        return json();
    }

    // false evita que el programa se cierre si el JSON tiene un error.
    json datos = json::parse(texto, NULL, false);

    if (datos.is_discarded()) {
        cout << "La respuesta de " << endpoint << " no contiene JSON valido." << endl;
        cout << "Inicio de la respuesta recibida: "
             << texto.substr(0, 200) << endl;
        return json();
    }

    return datos;
}

// Busca un arreglo tanto en la raiz como dentro de contenedores comunes.
json obtenerArreglo(json datos, string llave) {
    if (datos.is_array()) {
        return datos;
    }

    if (!datos.is_object()) {
        return json::array();
    }

    if (datos.contains(llave) && datos[llave].is_array()) {
        return datos[llave];
    }

    string contenedores[] = {"data", "grafo", "resultado", "result", "response"};

    for (int i = 0; i < 5; i++) {
        string contenedor = contenedores[i];

        if (!datos.contains(contenedor)) continue;

        if (datos[contenedor].is_array()) {
            return datos[contenedor];
        }

        if (datos[contenedor].is_object() &&
            datos[contenedor].contains(llave) &&
            datos[contenedor][llave].is_array()) {
            return datos[contenedor][llave];
        }
    }

    return json::array();
}


// Convierte valores JSON de texto, numero u objeto relacionado en una cadena utilizable.
string convertirJsonATexto(json valor) {
    if (valor.is_string()) {
        return valor.get<string>();
    }

    if (valor.is_number_integer()) {
        return to_string(valor.get<long long>());
    }

    if (valor.is_number_float()) {
        stringstream texto;
        texto << valor.get<double>();
        return texto.str();
    }

    // Algunas relaciones pueden venir como un objeto {"id": "..."}.
    if (valor.is_object()) {
        if (valor.contains("id")) return convertirJsonATexto(valor["id"]);
        if (valor.contains("codigo")) return convertirJsonATexto(valor["codigo"]);
        if (valor.contains("nombre")) return convertirJsonATexto(valor["nombre"]);
    }

    return "";
}


// Obtiene el primer campo de texto disponible entre varias llaves alternativas de un objeto JSON.
string leerCampoTexto(json dato, string llave1,
                       string llave2 = "", string llave3 = "") {
    if (dato.contains(llave1) && !dato[llave1].is_null()) {
        return convertirJsonATexto(dato[llave1]);
    }

    if (llave2 != "" && dato.contains(llave2) && !dato[llave2].is_null()) {
        return convertirJsonATexto(dato[llave2]);
    }

    if (llave3 != "" && dato.contains(llave3) && !dato[llave3].is_null()) {
        return convertirJsonATexto(dato[llave3]);
    }

    return "";
}


// Convierte un valor JSON numerico o textual en un numero de tipo double.
double convertirJsonANumero(json valor) {
    if (valor.is_number()) {
        return valor.get<double>();
    }

    if (valor.is_string()) {
        return atof(valor.get<string>().c_str());
    }

    return 0;
}


// Obtiene el primer campo numerico disponible entre varias llaves alternativas del JSON.
double leerCampoNumero(json dato, string llave1,
                        string llave2 = "", string llave3 = "") {
    if (dato.contains(llave1) && !dato[llave1].is_null()) {
        return convertirJsonANumero(dato[llave1]);
    }

    if (llave2 != "" && dato.contains(llave2) && !dato[llave2].is_null()) {
        return convertirJsonANumero(dato[llave2]);
    }

    if (llave3 != "" && dato.contains(llave3) && !dato[llave3].is_null()) {
        return convertirJsonANumero(dato[llave3]);
    }

    return 0;
}


// Interpreta un campo JSON como valor booleano aceptando booleanos, numeros y textos equivalentes.
bool leerCampoBool(json dato, string llave) {
    if (!dato.contains(llave)) return false;

    if (dato[llave].is_boolean()) {
        return dato[llave].get<bool>();
    }

    if (dato[llave].is_number_integer()) {
        return dato[llave].get<int>() == 1;
    }

    if (dato[llave].is_string()) {
        string valor = dato[llave].get<string>();
        return valor == "true" || valor == "1" || valor == "si";
    }

    return false;
}


// Normaliza claves y textos para comparar variantes como origen_id, origenId u ORIGEN-ID.
string normalizarTextoApi(string texto) {
    string resultado = "";

    for (int i = 0; i < (int)texto.length(); i++) {
        unsigned char caracter = (unsigned char)texto[i];

        if (isalnum(caracter)) {
            resultado += (char)tolower(caracter);
        }
    }

    return resultado;
}


// Elimina los espacios ubicados al inicio y al final de una cadena.
string quitarEspaciosExtremos(string texto) {
    int inicio = 0;
    int final = (int)texto.length() - 1;

    while (inicio <= final && isspace((unsigned char)texto[inicio])) inicio++;
    while (final >= inicio && isspace((unsigned char)texto[final])) final--;

    if (inicio > final) return "";
    return texto.substr(inicio, final - inicio + 1);
}

// Compara textos ignorando mayusculas, minusculas, espacios y separadores.
// Ejemplo: "GAL-001" y "gal 001" se consideran el mismo dato.
bool textosIguales(string texto1, string texto2) {
    return normalizarTextoApi(quitarEspaciosExtremos(texto1)) ==
           normalizarTextoApi(quitarEspaciosExtremos(texto2));
}


// Determina si una llave JSON coincide con alguna de las alternativas permitidas.
bool claveCoincideApi(string clave, const vector<string>& opciones) {
    string normalizada = normalizarTextoApi(clave);

    for (int i = 0; i < (int)opciones.size(); i++) {
        if (normalizada == normalizarTextoApi(opciones[i])) return true;
    }

    return false;
}

// Extrae un identificador de un valor sencillo o de un objeto relacionado.
string extraerReferenciaApi(json valor, int profundidad = 0) {
    if (profundidad > 4 || valor.is_null()) return "";

    if (valor.is_string() || valor.is_number()) {
        return quitarEspaciosExtremos(convertirJsonATexto(valor));
    }

    if (!valor.is_object()) return "";

    vector<string> llavesPreferidas = {
        "id", "_id", "codigo", "code", "clave",
        "galaxiaId", "idGalaxia", "nodoId", "idNodo",
        "naveId", "idNave", "rutaId", "idRuta", "uuid"
    };

    // Primero se buscan las claves que normalmente identifican una entidad.
    for (json::iterator it = valor.begin(); it != valor.end(); ++it) {
        if (claveCoincideApi(it.key(), llavesPreferidas)) {
            string respuesta = extraerReferenciaApi(it.value(), profundidad + 1);
            if (respuesta != "") return respuesta;
        }
    }

    // Si el objeto viene envuelto, se revisan sus valores internos.
    for (json::iterator it = valor.begin(); it != valor.end(); ++it) {
        if (it.value().is_object()) {
            string respuesta = extraerReferenciaApi(it.value(), profundidad + 1);
            if (respuesta != "") return respuesta;
        }
    }

    return "";
}


// Busca de forma flexible un dato textual dentro de un objeto JSON y posibles objetos anidados.
string buscarTextoApi(json dato, const vector<string>& llaves) {
    if (!dato.is_object()) return "";

    for (json::iterator it = dato.begin(); it != dato.end(); ++it) {
        if (claveCoincideApi(it.key(), llaves)) {
            string respuesta = extraerReferenciaApi(it.value());
            if (respuesta != "") return respuesta;
        }
    }

    // Algunas respuestas envuelven la entidad en ruta, arista, edge, data, etc.
    vector<string> contenedores = {
        "data", "ruta", "arista", "edge", "conexion", "registro", "resultado"
    };

    for (json::iterator it = dato.begin(); it != dato.end(); ++it) {
        if (it.value().is_object() && claveCoincideApi(it.key(), contenedores)) {
            string respuesta = buscarTextoApi(it.value(), llaves);
            if (respuesta != "") return respuesta;
        }
    }

    return "";
}


// Busca de forma flexible un dato numerico dentro de un objeto JSON y posibles objetos anidados.
double buscarNumeroApi(json dato, const vector<string>& llaves) {
    if (!dato.is_object()) return 0;

    for (json::iterator it = dato.begin(); it != dato.end(); ++it) {
        if (claveCoincideApi(it.key(), llaves)) {
            double numero = convertirJsonANumero(it.value());
            if (numero != 0) return numero;
        }
    }

    vector<string> contenedores = {
        "data", "ruta", "arista", "edge", "conexion", "registro", "resultado"
    };

    for (json::iterator it = dato.begin(); it != dato.end(); ++it) {
        if (it.value().is_object() && claveCoincideApi(it.key(), contenedores)) {
            double numero = buscarNumeroApi(it.value(), llaves);
            if (numero != 0) return numero;
        }
    }

    return 0;
}


// Busca un valor booleano dentro del JSON e indica si la propiedad fue encontrada.
bool buscarBoolApi(json dato, const vector<string>& llaves, bool& encontrado) {
    encontrado = false;
    if (!dato.is_object()) return false;

    for (json::iterator it = dato.begin(); it != dato.end(); ++it) {
        if (!claveCoincideApi(it.key(), llaves)) continue;

        encontrado = true;
        json valor = it.value();

        if (valor.is_boolean()) return valor.get<bool>();
        if (valor.is_number_integer()) return valor.get<int>() != 0;

        if (valor.is_string()) {
            string texto = normalizarTextoApi(valor.get<string>());
            return texto == "true" || texto == "1" || texto == "si" ||
                   texto == "dirigida" || texto == "directed";
        }
    }

    return false;
}


// Localiza una galaxia mediante ID, codigo o nombre, incluyendo referencias recibidas desde la API.
Galaxia* buscarGalaxiaFlexible(string valor) {
    valor = quitarEspaciosExtremos(valor);
    Galaxia* encontrada = buscarGalaxia(valor);
    if (encontrada != NULL) return encontrada;

    string normalizado = normalizarTextoApi(valor);
    Galaxia* actual = primeraGalaxia;

    while (actual != NULL) {
        if (normalizarTextoApi(actual->id) == normalizado ||
            normalizarTextoApi(actual->codigo) == normalizado ||
            normalizarTextoApi(actual->nombre) == normalizado) {
            return actual;
        }
        actual = actual->sigG;
    }

    return NULL;
}


// Localiza una nave mediante ID, codigo o nombre, incluyendo referencias recibidas desde la API.
Nave* buscarNaveFlexible(string valor) {
    valor = quitarEspaciosExtremos(valor);
    Nave* encontrada = buscarNave(valor);
    if (encontrada != NULL) return encontrada;

    string normalizado = normalizarTextoApi(valor);
    Nave* actual = primeraNave;

    while (actual != NULL) {
        if (normalizarTextoApi(actual->id) == normalizado ||
            normalizarTextoApi(actual->codigo) == normalizado ||
            normalizarTextoApi(actual->nombre) == normalizado) {
            return actual;
        }
        actual = actual->sigN;
    }

    return NULL;
}

// ============================================================================
// BUSQUEDAS EN LISTAS ENLAZADAS
// ============================================================================


// Recorre la lista enlazada y retorna la galaxia cuyo ID coincide con el valor indicado.
Galaxia* buscarGalaxiaPorId(string id) {
    Galaxia* actual = primeraGalaxia;

    while (actual != NULL) {
        if (textosIguales(actual->id, id)) return actual;
        actual = actual->sigG;
    }

    return NULL;
}


// Recorre la lista enlazada y retorna la galaxia cuyo codigo coincide con el valor indicado.
Galaxia* buscarGalaxiaPorCodigo(string codigo) {
    Galaxia* actual = primeraGalaxia;

    while (actual != NULL) {
        if (textosIguales(actual->codigo, codigo)) return actual;
        actual = actual->sigG;
    }

    return NULL;
}


// Recorre la lista enlazada y retorna la galaxia cuyo nombre coincide con el valor indicado.
Galaxia* buscarGalaxiaPorNombre(string nombre) {
    Galaxia* actual = primeraGalaxia;

    while (actual != NULL) {
        if (textosIguales(actual->nombre, nombre)) return actual;
        actual = actual->sigG;
    }

    return NULL;
}

// Permite consultar una galaxia por ID, codigo o nombre.
Galaxia* buscarGalaxia(string valor) {
    Galaxia* encontrada = buscarGalaxiaPorId(valor);
    if (encontrada != NULL) return encontrada;

    encontrada = buscarGalaxiaPorCodigo(valor);
    if (encontrada != NULL) return encontrada;

    return buscarGalaxiaPorNombre(valor);
}


// Recorre la lista de rutas y retorna la ruta que posee el identificador solicitado.
Ruta* buscarRuta(string id) {
    Ruta* actual = primeraRuta;

    while (actual != NULL) {
        if (textosIguales(actual->id, id)) return actual;
        actual = actual->sigR;
    }

    return NULL;
}


// Busca una ruta que conecte dos galaxias respetando si la conexion es dirigida o no dirigida.
Ruta* buscarRutaEntre(string origen, string destino) {
    Ruta* actual = primeraRuta;

    while (actual != NULL) {
        bool directa = textosIguales(actual->origen->id, origen) &&
                       textosIguales(actual->destino->id, destino);
        bool inversa = !actual->dirigida &&
                       textosIguales(actual->origen->id, destino) &&
                       textosIguales(actual->destino->id, origen);

        if (directa || inversa) return actual;
        actual = actual->sigR;
    }

    return NULL;
}

// Valida conexiones repetidas respetando si la nueva ruta es dirigida o no.
bool existeConexionDuplicada(Galaxia* origen, Galaxia* destino,
                              bool nuevaDirigida, Ruta* ignorar) {
    Ruta* actual = primeraRuta;

    while (actual != NULL) {
        if (actual != ignorar) {
            bool mismoSentido = actual->origen == origen && actual->destino == destino;
            bool sentidoContrario = actual->origen == destino && actual->destino == origen;

            if (mismoSentido) return true;

            // Una ruta no dirigida ocupa ambos sentidos.
            if (sentidoContrario && (!nuevaDirigida || !actual->dirigida)) {
                return true;
            }
        }
        actual = actual->sigR;
    }

    return false;
}


// Recorre la lista enlazada y retorna la nave cuyo ID coincide con el valor indicado.
Nave* buscarNavePorId(string id) {
    Nave* actual = primeraNave;

    while (actual != NULL) {
        if (textosIguales(actual->id, id)) return actual;
        actual = actual->sigN;
    }

    return NULL;
}


// Recorre la lista enlazada y retorna la nave cuyo codigo coincide con el valor indicado.
Nave* buscarNavePorCodigo(string codigo) {
    Nave* actual = primeraNave;

    while (actual != NULL) {
        if (textosIguales(actual->codigo, codigo)) return actual;
        actual = actual->sigN;
    }

    return NULL;
}


// Recorre la lista enlazada y retorna la nave cuyo nombre coincide con el valor indicado.
Nave* buscarNavePorNombre(string nombre) {
    Nave* actual = primeraNave;

    while (actual != NULL) {
        if (textosIguales(actual->nombre, nombre)) return actual;
        actual = actual->sigN;
    }

    return NULL;
}

// Permite consultar una nave por ID, codigo o nombre.
Nave* buscarNave(string valor) {
    Nave* encontrada = buscarNavePorId(valor);
    if (encontrada != NULL) return encontrada;

    encontrada = buscarNavePorCodigo(valor);
    if (encontrada != NULL) return encontrada;

    return buscarNavePorNombre(valor);
}


// Recorre el historial y retorna el viaje que posee el identificador solicitado.
Viaje* buscarViaje(string id) {
    Viaje* actual = primerViaje;

    while (actual != NULL) {
        if (textosIguales(actual->id, id)) return actual;
        actual = actual->sigV;
    }

    return NULL;
}

// ============================================================================
// INSERCION DE DATOS
// ============================================================================


// Valida los datos y agrega una galaxia al final de la lista enlazada sin permitir registros repetidos.
bool insertarGalaxia(string id, string codigo, string nombre,
                      double x, double y, double z, bool mostrarMensaje = true,
                      string tipo = "No especificado", string descripcion = "") {
    id = quitarEspaciosExtremos(id);
    codigo = quitarEspaciosExtremos(codigo);
    nombre = quitarEspaciosExtremos(nombre);
    tipo = quitarEspaciosExtremos(tipo);
    descripcion = quitarEspaciosExtremos(descripcion);

    if (textoVacio(id) || textoVacio(codigo) || textoVacio(nombre)) {
        if (mostrarMensaje) cout << "No se permiten datos vacios." << endl;
        return false;
    }

    if (textoContieneSeparadores(id) || textoContieneSeparadores(codigo) ||
        textoContieneSeparadores(nombre) || textoContieneSeparadores(tipo) ||
        textoContieneSeparadores(descripcion)) {
        if (mostrarMensaje) cout << "No se permiten los caracteres | o , en los textos." << endl;
        return false;
    }

    if (buscarGalaxiaPorId(id) != NULL) {
        if (mostrarMensaje) cout << "Error: el ID de galaxia ya esta registrado." << endl;
        return false;
    }

    if (buscarGalaxiaPorCodigo(codigo) != NULL) {
        if (mostrarMensaje) cout << "Error: el codigo de galaxia ya esta registrado." << endl;
        return false;
    }

    if (buscarGalaxiaPorNombre(nombre) != NULL) {
        if (mostrarMensaje) cout << "Error: el nombre de galaxia ya esta registrado." << endl;
        return false;
    }

    Galaxia* nueva = new Galaxia;
    nueva->id = id;
    nueva->codigo = codigo;
    nueva->nombre = nombre;
    nueva->tipo = tipo == "" ? "No especificado" : tipo;
    nueva->descripcion = descripcion;
    nueva->x = x;
    nueva->y = y;
    nueva->z = z;
    nueva->subListaArcos = NULL;
    nueva->sigG = NULL;

    if (primeraGalaxia == NULL) {
        primeraGalaxia = nueva;
    } else {
        Galaxia* actual = primeraGalaxia;
        while (actual->sigG != NULL) actual = actual->sigG;
        actual->sigG = nueva;
    }

    if (mostrarMensaje) cout << "Galaxia insertada correctamente." << endl;
    return true;
}


// Crea un arco y lo agrega a la lista de adyacencia de la galaxia de origen.
void insertarArco(Galaxia* origen, Galaxia* destino, Ruta* ruta) {
    if (origen == NULL || destino == NULL || ruta == NULL) return;

    Arco* nuevo = new Arco;
    nuevo->destino = destino;
    nuevo->ruta = ruta;
    nuevo->sigA = NULL;

    if (origen->subListaArcos == NULL) {
        origen->subListaArcos = nuevo;
    } else {
        Arco* actual = origen->subListaArcos;
        while (actual->sigA != NULL) actual = actual->sigA;
        actual->sigA = nuevo;
    }
}


// Valida y registra una ruta, relaciona sus galaxias y actualiza la lista de adyacencia del grafo.
bool insertarRuta(string id, string idOrigen, string idDestino,
                   double costo, bool dirigida, bool mostrarMensaje = true) {
    id = quitarEspaciosExtremos(id);
    idOrigen = quitarEspaciosExtremos(idOrigen);
    idDestino = quitarEspaciosExtremos(idDestino);

    Galaxia* origen = buscarGalaxia(idOrigen);
    Galaxia* destino = buscarGalaxia(idDestino);

    if (textoVacio(id)) {
        if (mostrarMensaje) cout << "El ID de la ruta no puede quedar vacio." << endl;
        return false;
    }

    if (textoContieneSeparadores(id)) {
        if (mostrarMensaje) cout << "El ID no puede contener | ni , ." << endl;
        return false;
    }

    if (buscarRuta(id) != NULL) {
        if (mostrarMensaje) cout << "Error: el ID de ruta ya esta registrado." << endl;
        return false;
    }

    if (origen == NULL || destino == NULL) {
        if (mostrarMensaje) cout << "La galaxia de origen o destino no existe." << endl;
        return false;
    }

    if (origen == destino) {
        if (mostrarMensaje) cout << "Una ruta no puede unir la misma galaxia." << endl;
        return false;
    }

    if (costo <= 0) {
        if (mostrarMensaje) cout << "El costo debe ser mayor que cero." << endl;
        return false;
    }

    if (existeConexionDuplicada(origen, destino, dirigida)) {
        if (mostrarMensaje) cout << "Error: ya existe una ruta entre esas galaxias." << endl;
        return false;
    }

    Ruta* nueva = new Ruta;
    nueva->id = id;
    nueva->origen = origen;
    nueva->destino = destino;
    nueva->costo = costo;
    nueva->dirigida = dirigida;
    nueva->sigR = NULL;

    if (primeraRuta == NULL) {
        primeraRuta = nueva;
    } else {
        Ruta* actual = primeraRuta;
        while (actual->sigR != NULL) actual = actual->sigR;
        actual->sigR = nueva;
    }

    insertarArco(origen, destino, nueva);
    if (!dirigida) insertarArco(destino, origen, nueva);

    if (mostrarMensaje) cout << "Ruta insertada correctamente." << endl;
    return true;
}


// Valida los datos y agrega una nave al final de su lista enlazada sin permitir duplicados.
bool insertarNave(string id, string codigo, string nombre,
                   bool mostrarMensaje = true) {
    id = quitarEspaciosExtremos(id);
    codigo = quitarEspaciosExtremos(codigo);
    nombre = quitarEspaciosExtremos(nombre);

    if (textoVacio(id) || textoVacio(codigo) || textoVacio(nombre)) {
        if (mostrarMensaje) cout << "No se permiten datos vacios." << endl;
        return false;
    }

    if (textoContieneSeparadores(id) || textoContieneSeparadores(codigo) ||
        textoContieneSeparadores(nombre)) {
        if (mostrarMensaje) cout << "No se permiten los caracteres | o , en los textos." << endl;
        return false;
    }

    if (buscarNavePorId(id) != NULL) {
        if (mostrarMensaje) cout << "Error: el ID de nave ya esta registrado." << endl;
        return false;
    }

    if (buscarNavePorCodigo(codigo) != NULL) {
        if (mostrarMensaje) cout << "Error: el codigo de nave ya esta registrado." << endl;
        return false;
    }

    if (buscarNavePorNombre(nombre) != NULL) {
        if (mostrarMensaje) cout << "Error: el nombre de nave ya esta registrado." << endl;
        return false;
    }

    Nave* nueva = new Nave;
    nueva->id = id;
    nueva->codigo = codigo;
    nueva->nombre = nombre;
    nueva->sigN = NULL;

    if (primeraNave == NULL) {
        primeraNave = nueva;
    } else {
        Nave* actual = primeraNave;
        while (actual->sigN != NULL) actual = actual->sigN;
        actual->sigN = nueva;
    }

    if (mostrarMensaje) cout << "Nave insertada correctamente." << endl;
    return true;
}


// Agrega al historial de un viaje una ruta activa mediante un nodo auxiliar PasoViaje.
void agregarPasoViaje(Viaje* viaje, Ruta* ruta) {
    if (viaje == NULL || ruta == NULL) return;

    PasoViaje* nuevo = new PasoViaje;
    nuevo->ruta = ruta;
    nuevo->idRuta = ruta->id;
    nuevo->siguiente = NULL;

    if (viaje->rutasUsadas == NULL) {
        viaje->rutasUsadas = nuevo;
    } else {
        PasoViaje* actual = viaje->rutasUsadas;
        while (actual->siguiente != NULL) actual = actual->siguiente;
        actual->siguiente = nuevo;
    }
}

// Guarda la referencia de una ruta historica aunque ya no forme parte
// del grafo activo recibido desde /grafo.
void agregarPasoViajePorId(Viaje* viaje, string idRuta) {
    if (viaje == NULL || textoVacio(idRuta)) return;

    PasoViaje* nuevo = new PasoViaje;
    nuevo->ruta = buscarRuta(idRuta);
    nuevo->idRuta = idRuta;
    nuevo->siguiente = NULL;

    if (viaje->rutasUsadas == NULL) {
        viaje->rutasUsadas = nuevo;
    } else {
        PasoViaje* actual = viaje->rutasUsadas;
        while (actual->siguiente != NULL) actual = actual->siguiente;
        actual->siguiente = nuevo;
    }
}


// Valida las asociaciones y agrega un nuevo registro de viaje al historial enlazado.
Viaje* insertarViaje(string id, string idNave, string idOrigen,
                      string idDestino, double costo, string fecha,
                      bool mostrarMensaje = true) {
    id = quitarEspaciosExtremos(id);
    idNave = quitarEspaciosExtremos(idNave);
    idOrigen = quitarEspaciosExtremos(idOrigen);
    idDestino = quitarEspaciosExtremos(idDestino);
    fecha = quitarEspaciosExtremos(fecha);

    Nave* nave = buscarNave(idNave);
    Galaxia* origen = buscarGalaxia(idOrigen);
    Galaxia* destino = buscarGalaxia(idDestino);

    if (textoVacio(id) || textoVacio(fecha)) {
        if (mostrarMensaje) cout << "El ID y la fecha del viaje son obligatorios." << endl;
        return NULL;
    }

    if (textoContieneSeparadores(id) || textoContieneSeparadores(fecha)) {
        if (mostrarMensaje) cout << "El ID y la fecha no pueden contener | ni , ." << endl;
        return NULL;
    }

    if (buscarViaje(id) != NULL) {
        if (mostrarMensaje) cout << "Error: el ID de viaje ya esta registrado." << endl;
        return NULL;
    }

    if (nave == NULL || origen == NULL || destino == NULL) {
        if (mostrarMensaje) cout << "No se pudo asociar la nave o las galaxias." << endl;
        return NULL;
    }

    if (origen == destino) {
        if (mostrarMensaje) cout << "El origen y el destino deben ser diferentes." << endl;
        return NULL;
    }

    if (costo <= 0) {
        if (mostrarMensaje) cout << "El costo del viaje debe ser mayor que cero." << endl;
        return NULL;
    }

    Viaje* nuevo = new Viaje;
    nuevo->id = id;
    nuevo->nave = nave;
    nuevo->origen = origen;
    nuevo->destino = destino;
    nuevo->costoTotal = costo;
    nuevo->fecha = fecha;
    nuevo->rutasUsadas = NULL;
    nuevo->sigV = NULL;

    if (primerViaje == NULL) {
        primerViaje = nuevo;
    } else {
        Viaje* actual = primerViaje;
        while (actual->sigV != NULL) actual = actual->sigV;
        actual->sigV = nuevo;
    }

    if (mostrarMensaje) cout << "Viaje registrado correctamente." << endl;
    return nuevo;
}

// ============================================================================
// MOSTRAR DATOS
// ============================================================================


// Muestra todos los datos almacenados de cada galaxia registrada en el sistema.
void mostrarGalaxias() {
    Galaxia* actual = primeraGalaxia;

    if (actual == NULL) {
        cout << "No hay galaxias registradas." << endl;
        return;
    }

    cout << "\n================ GALAXIAS ================" << endl;
    while (actual != NULL) {
        cout << "ID: " << actual->id
             << " | Codigo: " << actual->codigo
             << " | Nombre: " << actual->nombre
             << " | Tipo: " << actual->tipo
             << " | Coordenadas: (" << actual->x << ", "
             << actual->y << ", " << actual->z << ")" << endl;
        cout << "Descripcion: "
             << (actual->descripcion == "" ? "No disponible" : actual->descripcion)
             << endl;
        actual = actual->sigG;
    }
}


// Muestra las rutas registradas, sus extremos, costo y tipo de direccion.
void mostrarRutas() {
    Ruta* actual = primeraRuta;

    if (actual == NULL) {
        cout << "No hay rutas registradas." << endl;
        return;
    }

    cout << "\n================= RUTAS =================" << endl;
    while (actual != NULL) {
        cout << "ID: " << actual->id
             << " | " << actual->origen->nombre
             << " -> " << actual->destino->nombre
             << " | Costo: " << actual->costo
             << " | Tipo: " << (actual->dirigida ? "Dirigida" : "No dirigida")
             << endl;
        actual = actual->sigR;
    }
}


// Muestra el ID, codigo y nombre de todas las naves registradas.
void mostrarNaves() {
    Nave* actual = primeraNave;

    if (actual == NULL) {
        cout << "No hay naves registradas." << endl;
        return;
    }

    cout << "\n================= NAVES =================" << endl;
    while (actual != NULL) {
        cout << "ID: " << actual->id
             << " | Codigo: " << actual->codigo
             << " | Nombre: " << actual->nombre << endl;
        actual = actual->sigN;
    }
}


// Muestra los datos generales y las rutas utilizadas por un viaje especifico.
void mostrarViaje(Viaje* viaje) {
    if (viaje == NULL) return;

    cout << "ID viaje: " << viaje->id
         << " | Nave: " << viaje->nave->nombre
         << " | " << viaje->origen->nombre
         << " -> " << viaje->destino->nombre
         << " | Costo: " << viaje->costoTotal
         << " | Fecha: " << viaje->fecha << endl;

    cout << "Rutas utilizadas: ";
    PasoViaje* paso = viaje->rutasUsadas;

    if (paso == NULL) cout << "No especificadas";

    while (paso != NULL) {
        cout << paso->idRuta;
        if (paso->ruta == NULL) cout << " (historica/inactiva)";
        if (paso->siguiente != NULL) cout << " -> ";
        paso = paso->siguiente;
    }
    cout << endl;
}


// Recorre y muestra todos los viajes almacenados en el historial.
void mostrarHistorial() {
    Viaje* actual = primerViaje;

    if (actual == NULL) {
        cout << "No hay viajes registrados." << endl;
        return;
    }

    cout << "\n=============== HISTORIAL ===============" << endl;
    while (actual != NULL) {
        mostrarViaje(actual);
        cout << "-----------------------------------------" << endl;
        actual = actual->sigV;
    }
}


// Filtra el historial y muestra unicamente los viajes asociados con una nave.
void mostrarHistorialNave(string idNave) {
    Nave* nave = buscarNave(idNave);

    if (nave == NULL) {
        cout << "La nave no existe." << endl;
        return;
    }

    bool encontrado = false;
    Viaje* actual = primerViaje;

    cout << "\nHistorial de la nave " << nave->nombre << ":" << endl;

    while (actual != NULL) {
        if (actual->nave == nave) {
            mostrarViaje(actual);
            encontrado = true;
        }
        actual = actual->sigV;
    }

    if (!encontrado) cout << "La nave no posee viajes registrados." << endl;
}

// ============================================================================
// MANEJO DE ARCOS
// ============================================================================


// Elimina de memoria todos los arcos pertenecientes a la lista de adyacencia de una galaxia.
void liberarArcos(Galaxia* galaxia) {
    Arco* actual = galaxia->subListaArcos;

    while (actual != NULL) {
        Arco* borrar = actual;
        actual = actual->sigA;
        delete borrar;
    }

    galaxia->subListaArcos = NULL;
}


// Elimina las listas de adyacencia actuales y las vuelve a construir a partir de las rutas registradas.
void reconstruirGrafo() {
    Galaxia* galaxia = primeraGalaxia;

    while (galaxia != NULL) {
        liberarArcos(galaxia);
        galaxia = galaxia->sigG;
    }

    Ruta* ruta = primeraRuta;
    while (ruta != NULL) {
        insertarArco(ruta->origen, ruta->destino, ruta);
        if (!ruta->dirigida) insertarArco(ruta->destino, ruta->origen, ruta);
        ruta = ruta->sigR;
    }
}

// ============================================================================
// ALGORITMO DE DIJKSTRA
// ============================================================================


// Copia los punteros de la lista enlazada de galaxias a un vector auxiliar para los algoritmos.
vector<Galaxia*> obtenerVectorGalaxias() {
    vector<Galaxia*> lista;
    Galaxia* actual = primeraGalaxia;

    while (actual != NULL) {
        lista.push_back(actual);
        actual = actual->sigG;
    }

    return lista;
}


// Retorna la posicion que ocupa una galaxia dentro del vector auxiliar o -1 si no existe.
int indiceGalaxia(const vector<Galaxia*>& galaxias, Galaxia* buscada) {
    for (int i = 0; i < (int)galaxias.size(); i++) {
        if (galaxias[i] == buscada) return i;
    }
    return -1;
}


// Calcula el camino de menor costo entre dos galaxias y retorna las galaxias y rutas que lo componen.
ResultadoCamino dijkstra(string idOrigen, string idDestino) {
    ResultadoCamino resultado;
    resultado.encontrado = false;
    resultado.costoTotal = 0;

    Galaxia* origen = buscarGalaxia(idOrigen);
    Galaxia* destino = buscarGalaxia(idDestino);

    if (origen == NULL || destino == NULL) {
        cout << "La galaxia de origen o destino no existe." << endl;
        return resultado;
    }

    vector<Galaxia*> vertices = obtenerVectorGalaxias();
    int n = vertices.size();

    vector<double> distancia(n, INFINITO);
    vector<bool> visitado(n, false);
    vector<int> anterior(n, -1);
    vector<Ruta*> rutaAnterior(n, NULL);

    int inicio = indiceGalaxia(vertices, origen);
    int final = indiceGalaxia(vertices, destino);
    distancia[inicio] = 0;

    // Se busca el vertice no visitado con menor distancia.
    for (int paso = 0; paso < n; paso++) {
        int menor = -1;

        for (int i = 0; i < n; i++) {
            if (!visitado[i] &&
                (menor == -1 || distancia[i] < distancia[menor])) {
                menor = i;
            }
        }

        if (menor == -1 || distancia[menor] == INFINITO) break;

        visitado[menor] = true;
        if (menor == final) break;

        Arco* arco = vertices[menor]->subListaArcos;

        while (arco != NULL) {
            int vecino = indiceGalaxia(vertices, arco->destino);

            if (vecino == -1) {
                arco = arco->sigA;
                continue;
            }

            double nuevaDistancia = distancia[menor] + arco->ruta->costo;

            if (!visitado[vecino] && nuevaDistancia < distancia[vecino]) {
                distancia[vecino] = nuevaDistancia;
                anterior[vecino] = menor;
                rutaAnterior[vecino] = arco->ruta;
            }

            arco = arco->sigA;
        }
    }

    if (distancia[final] == INFINITO) {
        cout << "No existe una ruta entre las galaxias indicadas." << endl;
        return resultado;
    }

    // Reconstruccion del camino desde el destino hasta el origen.
    vector<Galaxia*> galaxiasInvertidas;
    vector<Ruta*> rutasInvertidas;
    int actual = final;

    while (actual != -1) {
        galaxiasInvertidas.push_back(vertices[actual]);
        if (rutaAnterior[actual] != NULL) {
            rutasInvertidas.push_back(rutaAnterior[actual]);
        }
        actual = anterior[actual];
    }

    for (int i = galaxiasInvertidas.size() - 1; i >= 0; i--) {
        resultado.galaxias.push_back(galaxiasInvertidas[i]);
    }

    for (int i = rutasInvertidas.size() - 1; i >= 0; i--) {
        resultado.rutas.push_back(rutasInvertidas[i]);
    }

    resultado.encontrado = true;
    resultado.costoTotal = distancia[final];
    cantidadRutasCalculadas++;
    return resultado;
}


// Presenta en pantalla el recorrido y costo obtenidos por el algoritmo de Dijkstra.
void mostrarResultadoDijkstra(ResultadoCamino resultado) {
    if (!resultado.encontrado) return;

    cout << "Ruta de menor costo: ";
    for (int i = 0; i < (int)resultado.galaxias.size(); i++) {
        cout << resultado.galaxias[i]->nombre;
        if (i < (int)resultado.galaxias.size() - 1) cout << " -> ";
    }

    cout << endl << "Costo total: " << resultado.costoTotal << endl;
    cout << "Codigos de ruta: ";

    for (int i = 0; i < (int)resultado.rutas.size(); i++) {
        cout << resultado.rutas[i]->id;
        if (i < (int)resultado.rutas.size() - 1) cout << ", ";
    }
    cout << endl;
}

// ============================================================================
// KRUSKAL MODIFICADO: RECIBE LAS ARISTAS EN ORDEN ALEATORIO
// ============================================================================


// Encuentra el conjunto al que pertenece una posicion dentro de la estructura auxiliar de Kruskal.
int buscarPadre(vector<int>& padre, int posicion) {
    while (padre[posicion] != posicion) {
        posicion = padre[posicion];
    }
    return posicion;
}


// Une dos conjuntos diferentes para registrar una nueva conexion sin formar ciclos.
void unirConjuntos(vector<int>& padre, int conjuntoA, int conjuntoB) {
    int raizA = buscarPadre(padre, conjuntoA);
    int raizB = buscarPadre(padre, conjuntoB);
    padre[raizB] = raizA;
}


// Crea una copia de las rutas y mezcla su orden como respaldo para el Kruskal modificado.
vector<Ruta*> crearOrdenAleatorioLocal() {
    vector<Ruta*> rutas;
    Ruta* actual = primeraRuta;

    while (actual != NULL) {
        rutas.push_back(actual);
        actual = actual->sigR;
    }

    // Mezcla sencilla de posiciones, sin utilizar algoritmos avanzados.
    for (int i = rutas.size() - 1; i > 0; i--) {
        int posicion = rand() % (i + 1);
        Ruta* temporal = rutas[i];
        rutas[i] = rutas[posicion];
        rutas[posicion] = temporal;
    }

    return rutas;
}


// Construye un arbol de expansion usando el orden aleatorio recibido de la API o generado localmente.
vector<Ruta*> kruskalModificado() {
    vector<Galaxia*> galaxias = obtenerVectorGalaxias();
    vector<Ruta*> aristas = ordenKruskal;
    vector<Ruta*> arbol;

    if (aristas.size() == 0) {
        aristas = crearOrdenAleatorioLocal();
    } else {
        // Si el endpoint no incluyera alguna ruta, se agrega al final para
        // que Kruskal pueda intentar conectar todo el grafo.
        Ruta* rutaLocal = primeraRuta;
        while (rutaLocal != NULL) {
            bool encontrada = false;
            for (int i = 0; i < (int)aristas.size(); i++) {
                if (aristas[i] == rutaLocal) encontrada = true;
            }
            if (!encontrada) aristas.push_back(rutaLocal);
            rutaLocal = rutaLocal->sigR;
        }
    }

    vector<int> padre(galaxias.size());
    for (int i = 0; i < (int)padre.size(); i++) padre[i] = i;

    for (int i = 0; i < (int)aristas.size(); i++) {
        Ruta* ruta = aristas[i];
        int origen = indiceGalaxia(galaxias, ruta->origen);
        int destino = indiceGalaxia(galaxias, ruta->destino);

        int conjuntoOrigen = buscarPadre(padre, origen);
        int conjuntoDestino = buscarPadre(padre, destino);

        if (conjuntoOrigen != conjuntoDestino) {
            arbol.push_back(ruta);
            unirConjuntos(padre, conjuntoOrigen, conjuntoDestino);
        }

        if ((int)arbol.size() == (int)galaxias.size() - 1) break;
    }

    return arbol;
}


// Muestra las conexiones seleccionadas por Kruskal y calcula el costo total del arbol.
void mostrarKruskal() {
    vector<Ruta*> arbol = kruskalModificado();
    double costoTotal = 0;

    cout << "\n======= ARBOL DE CONEXIONES: KRUSKAL =======" << endl;

    for (int i = 0; i < (int)arbol.size(); i++) {
        cout << arbol[i]->origen->nombre << " -- "
             << arbol[i]->destino->nombre
             << " | Costo: " << arbol[i]->costo << endl;
        costoTotal += arbol[i]->costo;
    }

    cout << "Costo total del arbol: " << costoTotal << endl;

    vector<Galaxia*> galaxias = obtenerVectorGalaxias();
    if (galaxias.size() > 0 && arbol.size() != galaxias.size() - 1) {
        cout << "Aviso: el grafo no quedo completamente conectado." << endl;
    }
}

// ============================================================================
// CONSULTAS
// ============================================================================


// Recorre la lista de adyacencia y muestra todas las salidas disponibles desde una galaxia.
void mostrarRutasDesdeGalaxia(string id) {
    Galaxia* galaxia = buscarGalaxia(id);

    if (galaxia == NULL) {
        cout << "La galaxia no existe." << endl;
        return;
    }

    Arco* arco = galaxia->subListaArcos;

    if (arco == NULL) {
        cout << "La galaxia no posee rutas disponibles." << endl;
        return;
    }

    cout << "\nRutas posibles desde " << galaxia->nombre << ":" << endl;

    while (arco != NULL) {
        cout << "- Hacia " << arco->destino->nombre
             << " | Ruta: " << arco->ruta->id
             << " | Costo: " << arco->ruta->costo << endl;
        arco = arco->sigA;
    }
}


// Solicita origen y destino, ejecuta Dijkstra y muestra la ruta de menor costo.
void consultarRutaMinima() {
    string origen = leerTexto("ID o codigo de galaxia origen: ");
    string destino = leerTexto("ID o codigo de galaxia destino: ");
    ResultadoCamino resultado = dijkstra(origen, destino);
    mostrarResultadoDijkstra(resultado);
}

// ============================================================================
// REGISTRO DE VIAJES
// ============================================================================


// Registra un viaje nuevo utilizando la ruta minima calculada entre las galaxias indicadas.
void registrarViaje() {
    string id = quitarEspaciosExtremos(leerTexto("ID del viaje: "));

    if (buscarViaje(id) != NULL) {
        cout << "Error: el ID de viaje ya esta registrado." << endl;
        return;
    }

    string idNave = quitarEspaciosExtremos(
        leerTexto("ID, codigo o nombre de nave: ")
    );
    string idOrigen = quitarEspaciosExtremos(
        leerTexto("ID, codigo o nombre de galaxia origen: ")
    );
    string idDestino = quitarEspaciosExtremos(
        leerTexto("ID, codigo o nombre de galaxia destino: ")
    );
    string fecha = quitarEspaciosExtremos(leerTexto("Fecha del viaje: "));

    ResultadoCamino resultado = dijkstra(idOrigen, idDestino);

    if (!resultado.encontrado) return;

    Nave* nave = buscarNave(idNave);
    Galaxia* origen = buscarGalaxia(idOrigen);
    Galaxia* destino = buscarGalaxia(idDestino);

    if (nave == NULL) {
        cout << "La nave no existe." << endl;
        return;
    }

    Viaje* nuevo = insertarViaje(id, nave->id, origen->id, destino->id,
                                 resultado.costoTotal, fecha, false);

    if (nuevo == NULL) return;

    for (int i = 0; i < (int)resultado.rutas.size(); i++) {
        agregarPasoViaje(nuevo, resultado.rutas[i]);
    }

    guardarDatosLocales();
    cout << "Viaje registrado con la ruta de menor costo." << endl;
    mostrarResultadoDijkstra(resultado);
}

// ============================================================================
// MODIFICAR Y ELIMINAR GALAXIAS
// ============================================================================


// Verifica si una galaxia participa como origen o destino en alguna ruta activa.
bool galaxiaTieneRutas(Galaxia* galaxia) {
    Ruta* ruta = primeraRuta;

    while (ruta != NULL) {
        if (ruta->origen == galaxia || ruta->destino == galaxia) return true;
        ruta = ruta->sigR;
    }

    return false;
}

// Verifica si una galaxia aparece como origen o destino de un viaje guardado.
bool galaxiaUsadaEnHistorial(Galaxia* galaxia) {
    Viaje* viaje = primerViaje;

    while (viaje != NULL) {
        if (viaje->origen == galaxia || viaje->destino == galaxia) return true;
        viaje = viaje->sigV;
    }

    return false;
}


// Actualiza los datos editables de una galaxia y valida que codigo y nombre permanezcan unicos.
void modificarGalaxia() {
    string valorBusqueda = quitarEspaciosExtremos(
        leerTexto("ID, codigo o nombre de galaxia a modificar: ")
    );
    Galaxia* galaxia = buscarGalaxia(valorBusqueda);

    if (galaxia == NULL) {
        cout << "La galaxia no existe." << endl;
        return;
    }

    string codigo = quitarEspaciosExtremos(leerTexto("Nuevo codigo: "));
    string nombre = quitarEspaciosExtremos(leerTexto("Nuevo nombre: "));
    string tipo = quitarEspaciosExtremos(leerTexto("Nuevo tipo: "));
    string descripcion = quitarEspaciosExtremos(leerTexto("Nueva descripcion: "));

    if (textoVacio(codigo) || textoVacio(nombre) || textoVacio(tipo)) {
        cout << "El codigo, nombre y tipo no pueden quedar vacios." << endl;
        return;
    }

    if (textoContieneSeparadores(codigo) || textoContieneSeparadores(nombre) ||
        textoContieneSeparadores(tipo) || textoContieneSeparadores(descripcion)) {
        cout << "No se permiten los caracteres | o , en los textos." << endl;
        return;
    }

    Galaxia* codigoRepetido = buscarGalaxiaPorCodigo(codigo);
    if (codigoRepetido != NULL && codigoRepetido != galaxia) {
        cout << "Error: el codigo de galaxia ya pertenece a otro registro." << endl;
        return;
    }

    Galaxia* nombreRepetido = buscarGalaxiaPorNombre(nombre);
    if (nombreRepetido != NULL && nombreRepetido != galaxia) {
        cout << "Error: el nombre de galaxia ya pertenece a otro registro." << endl;
        return;
    }

    galaxia->codigo = codigo;
    galaxia->nombre = nombre;
    galaxia->tipo = tipo;
    galaxia->descripcion = descripcion;
    galaxia->x = leerDecimal("Nueva coordenada X: ");
    galaxia->y = leerDecimal("Nueva coordenada Y: ");
    galaxia->z = leerDecimal("Nueva coordenada Z: ");

    guardarDatosLocales();
    cout << "Galaxia modificada correctamente." << endl;
}


// Elimina una galaxia solamente cuando no posee rutas ni viajes asociados.
void eliminarGalaxia() {
    string id = quitarEspaciosExtremos(leerTexto("ID, codigo o nombre de galaxia a eliminar: "));
    Galaxia* actual = primeraGalaxia;
    Galaxia* anterior = NULL;

    while (actual != NULL && actual != buscarGalaxia(id)) {
        anterior = actual;
        actual = actual->sigG;
    }

    if (actual == NULL) {
        cout << "La galaxia no existe." << endl;
        return;
    }

    if (galaxiaTieneRutas(actual)) {
        cout << "No se puede eliminar: tiene rutas asociadas." << endl;
        return;
    }

    if (galaxiaUsadaEnHistorial(actual)) {
        cout << "No se puede eliminar: aparece en el historial de viajes." << endl;
        return;
    }

    if (anterior == NULL) primeraGalaxia = actual->sigG;
    else anterior->sigG = actual->sigG;

    liberarArcos(actual);
    delete actual;
    guardarDatosLocales();
    cout << "Galaxia eliminada correctamente." << endl;
}

// ============================================================================
// MODIFICAR Y ELIMINAR RUTAS
// ============================================================================


// Verifica si una ruta activa aparece en los pasos almacenados de algun viaje.
bool rutaUsadaEnHistorial(Ruta* ruta) {
    Viaje* viaje = primerViaje;

    while (viaje != NULL) {
        PasoViaje* paso = viaje->rutasUsadas;
        while (paso != NULL) {
            if (paso->ruta == ruta || paso->idRuta == ruta->id) return true;
            paso = paso->siguiente;
        }
        viaje = viaje->sigV;
    }

    return false;
}


// Actualiza extremos, costo y direccion de una ruta conservando la integridad del grafo.
void modificarRuta() {
    string id = quitarEspaciosExtremos(leerTexto("ID de ruta a modificar: "));
    Ruta* ruta = buscarRuta(id);

    if (ruta == NULL) {
        cout << "La ruta no existe." << endl;
        return;
    }

    if (rutaUsadaEnHistorial(ruta)) {
        cout << "No se puede modificar: la ruta aparece en el historial de viajes." << endl;
        return;
    }

    string idOrigen = quitarEspaciosExtremos(leerTexto("Nuevo ID, codigo o nombre de origen: "));
    string idDestino = quitarEspaciosExtremos(leerTexto("Nuevo ID, codigo o nombre de destino: "));
    Galaxia* origen = buscarGalaxia(idOrigen);
    Galaxia* destino = buscarGalaxia(idDestino);

    if (origen == NULL || destino == NULL || origen == destino) {
        cout << "Las galaxias indicadas no son validas." << endl;
        return;
    }

    double costo = leerDecimal("Nuevo costo: ");
    if (costo <= 0) {
        cout << "El costo debe ser mayor que cero." << endl;
        return;
    }

    int tipo;
    do {
        tipo = leerEntero("Tipo de ruta (1 dirigida, 2 no dirigida): ");
        if (tipo != 1 && tipo != 2) cout << "Digite 1 o 2." << endl;
    } while (tipo != 1 && tipo != 2);

    bool nuevaDirigida = tipo == 1;
    if (existeConexionDuplicada(origen, destino, nuevaDirigida, ruta)) {
        cout << "Error: ya existe una ruta entre esas galaxias." << endl;
        return;
    }

    ruta->origen = origen;
    ruta->destino = destino;
    ruta->costo = costo;
    ruta->dirigida = nuevaDirigida;

    reconstruirGrafo();
    ordenKruskal.clear();
    guardarDatosLocales();
    cout << "Ruta modificada correctamente." << endl;
}


// Elimina una ruta no utilizada en el historial y reconstruye las listas de adyacencia.
void eliminarRuta() {
    string id = quitarEspaciosExtremos(leerTexto("ID de ruta a eliminar: "));
    Ruta* actual = primeraRuta;
    Ruta* anterior = NULL;

    while (actual != NULL && actual->id != id) {
        anterior = actual;
        actual = actual->sigR;
    }

    if (actual == NULL) {
        cout << "La ruta no existe." << endl;
        return;
    }

    if (rutaUsadaEnHistorial(actual)) {
        cout << "No se puede eliminar: aparece en el historial de viajes." << endl;
        return;
    }

    if (anterior == NULL) primeraRuta = actual->sigR;
    else anterior->sigR = actual->sigR;

    delete actual;
    reconstruirGrafo();
    ordenKruskal.clear();
    guardarDatosLocales();
    cout << "Ruta eliminada correctamente." << endl;
}

// ============================================================================
// MODIFICAR Y ELIMINAR NAVES
// ============================================================================


// Verifica si una nave se encuentra asociada con uno o mas viajes registrados.
bool naveTieneViajes(Nave* nave) {
    Viaje* viaje = primerViaje;

    while (viaje != NULL) {
        if (viaje->nave == nave) return true;
        viaje = viaje->sigV;
    }

    return false;
}


// Actualiza codigo y nombre de una nave sin permitir valores repetidos.
void modificarNave() {
    string valorBusqueda = quitarEspaciosExtremos(
        leerTexto("ID, codigo o nombre de nave a modificar: ")
    );
    Nave* nave = buscarNave(valorBusqueda);

    if (nave == NULL) {
        cout << "La nave no existe." << endl;
        return;
    }

    string codigo = quitarEspaciosExtremos(leerTexto("Nuevo codigo: "));
    string nombre = quitarEspaciosExtremos(leerTexto("Nuevo nombre: "));

    if (textoVacio(codigo) || textoVacio(nombre)) {
        cout << "El codigo y el nombre no pueden quedar vacios." << endl;
        return;
    }

    if (textoContieneSeparadores(codigo) || textoContieneSeparadores(nombre)) {
        cout << "No se permiten los caracteres | o , en los textos." << endl;
        return;
    }

    Nave* codigoRepetido = buscarNavePorCodigo(codigo);
    if (codigoRepetido != NULL && codigoRepetido != nave) {
        cout << "Error: el codigo de nave ya pertenece a otro registro." << endl;
        return;
    }

    Nave* nombreRepetido = buscarNavePorNombre(nombre);
    if (nombreRepetido != NULL && nombreRepetido != nave) {
        cout << "Error: el nombre de nave ya pertenece a otro registro." << endl;
        return;
    }

    nave->codigo = codigo;
    nave->nombre = nombre;
    guardarDatosLocales();
    cout << "Nave modificada correctamente." << endl;
}


// Elimina una nave solamente cuando no posee viajes asociados.
void eliminarNave() {
    string id = quitarEspaciosExtremos(leerTexto("ID, codigo o nombre de nave a eliminar: "));
    Nave* objetivo = buscarNave(id);
    Nave* actual = primeraNave;
    Nave* anterior = NULL;

    if (objetivo == NULL) {
        cout << "La nave no existe." << endl;
        return;
    }

    if (naveTieneViajes(objetivo)) {
        cout << "No se puede eliminar: posee viajes registrados." << endl;
        return;
    }

    while (actual != NULL && actual != objetivo) {
        anterior = actual;
        actual = actual->sigN;
    }

    if (anterior == NULL) primeraNave = actual->sigN;
    else anterior->sigN = actual->sigN;

    delete actual;
    guardarDatosLocales();
    cout << "Nave eliminada correctamente." << endl;
}

// ============================================================================
// ELIMINAR VIAJE
// ============================================================================


// Elimina de memoria todos los nodos PasoViaje pertenecientes a un recorrido.
void liberarPasos(PasoViaje* paso) {
    while (paso != NULL) {
        PasoViaje* borrar = paso;
        paso = paso->siguiente;
        delete borrar;
    }
}

// Modifica un viaje y vuelve a calcular su camino de menor costo.
void modificarViaje() {
    string id = quitarEspaciosExtremos(leerTexto("ID del viaje a modificar: "));
    Viaje* viaje = buscarViaje(id);

    if (viaje == NULL) {
        cout << "El viaje no existe." << endl;
        return;
    }

    string idNave = quitarEspaciosExtremos(
        leerTexto("Nuevo ID, codigo o nombre de nave: ")
    );
    string idOrigen = quitarEspaciosExtremos(
        leerTexto("Nuevo ID, codigo o nombre de galaxia origen: ")
    );
    string idDestino = quitarEspaciosExtremos(
        leerTexto("Nuevo ID, codigo o nombre de galaxia destino: ")
    );
    string fecha = quitarEspaciosExtremos(leerTexto("Nueva fecha del viaje: "));

    Nave* nave = buscarNave(idNave);
    Galaxia* origen = buscarGalaxia(idOrigen);
    Galaxia* destino = buscarGalaxia(idDestino);

    if (nave == NULL || origen == NULL || destino == NULL) {
        cout << "La nave o alguna galaxia no existe." << endl;
        return;
    }

    if (origen == destino) {
        cout << "El origen y el destino deben ser diferentes." << endl;
        return;
    }

    if (textoVacio(fecha) || textoContieneSeparadores(fecha)) {
        cout << "La fecha es obligatoria y no puede contener | ni , ." << endl;
        return;
    }

    ResultadoCamino resultado = dijkstra(origen->id, destino->id);
    if (!resultado.encontrado) return;

    liberarPasos(viaje->rutasUsadas);
    viaje->rutasUsadas = NULL;
    viaje->nave = nave;
    viaje->origen = origen;
    viaje->destino = destino;
    viaje->costoTotal = resultado.costoTotal;
    viaje->fecha = fecha;

    for (int i = 0; i < (int)resultado.rutas.size(); i++) {
        agregarPasoViaje(viaje, resultado.rutas[i]);
    }

    guardarDatosLocales();
    cout << "Viaje modificado correctamente." << endl;
}


// Retira un viaje del historial y libera los pasos de ruta almacenados.
void eliminarViaje() {
    string id = quitarEspaciosExtremos(leerTexto("ID del viaje a eliminar: "));
    Viaje* actual = primerViaje;
    Viaje* anterior = NULL;

    while (actual != NULL && actual->id != id) {
        anterior = actual;
        actual = actual->sigV;
    }

    if (actual == NULL) {
        cout << "El viaje no existe." << endl;
        return;
    }

    if (anterior == NULL) primerViaje = actual->sigV;
    else anterior->sigV = actual->sigV;

    liberarPasos(actual->rutasUsadas);
    delete actual;
    guardarDatosLocales();
    cout << "Viaje eliminado correctamente." << endl;
}

// ============================================================================
// ARCHIVOS LOCALES
// ============================================================================


// Escribe todas las galaxias en el archivo galaxias.txt usando un formato separado por barras.
void guardarGalaxias() {
    ofstream archivo("galaxias.txt");

    if (!archivo.is_open()) {
        cout << "No se pudo abrir el archivo de salida." << endl;
        return;
    }
    Galaxia* actual = primeraGalaxia;

    while (actual != NULL) {
        archivo << actual->id << "|" << actual->codigo << "|"
                << actual->nombre << "|" << actual->x << "|"
                << actual->y << "|" << actual->z << "|"
                << actual->tipo << "|" << actual->descripcion << endl;
        actual = actual->sigG;
    }

    archivo.close();
}


// Escribe todas las rutas y sus relaciones en el archivo rutas.txt.
void guardarRutas() {
    ofstream archivo("rutas.txt");

    if (!archivo.is_open()) {
        cout << "No se pudo abrir el archivo de salida." << endl;
        return;
    }
    Ruta* actual = primeraRuta;

    while (actual != NULL) {
        archivo << actual->id << "|" << actual->origen->id << "|"
                << actual->destino->id << "|" << actual->costo << "|"
                << actual->dirigida << endl;
        actual = actual->sigR;
    }

    archivo.close();
}


// Escribe todas las naves registradas en el archivo naves.txt.
void guardarNaves() {
    ofstream archivo("naves.txt");

    if (!archivo.is_open()) {
        cout << "No se pudo abrir el archivo de salida." << endl;
        return;
    }
    Nave* actual = primeraNave;

    while (actual != NULL) {
        archivo << actual->id << "|" << actual->codigo << "|"
                << actual->nombre << endl;
        actual = actual->sigN;
    }

    archivo.close();
}


// Escribe los viajes y las rutas utilizadas en el archivo historial.txt.
void guardarHistorial() {
    ofstream archivo("historial.txt");

    if (!archivo.is_open()) {
        cout << "No se pudo abrir el archivo de salida." << endl;
        return;
    }
    Viaje* actual = primerViaje;

    while (actual != NULL) {
        archivo << actual->id << "|" << actual->nave->id << "|"
                << actual->origen->id << "|" << actual->destino->id << "|"
                << actual->costoTotal << "|" << actual->fecha << "|";

        PasoViaje* paso = actual->rutasUsadas;
        while (paso != NULL) {
            archivo << paso->idRuta;
            if (paso->siguiente != NULL) archivo << ",";
            paso = paso->siguiente;
        }

        archivo << endl;
        actual = actual->sigV;
    }

    archivo.close();
}


// Centraliza el guardado de galaxias, rutas, naves e historial en sus archivos respectivos.
void guardarDatosLocales() {
    guardarGalaxias();
    guardarRutas();
    guardarNaves();
    guardarHistorial();
}


// Lee galaxias.txt y reconstruye la lista enlazada de galaxias.
void cargarGalaxiasArchivo() {
    ifstream archivo("galaxias.txt");
    string linea;

    while (getline(archivo, linea)) {
        stringstream datos(linea);
        string id, codigo, nombre, x, y, z, tipo, descripcion;

        getline(datos, id, '|');
        getline(datos, codigo, '|');
        getline(datos, nombre, '|');
        getline(datos, x, '|');
        getline(datos, y, '|');
        getline(datos, z, '|');
        getline(datos, tipo, '|');
        getline(datos, descripcion, '|');

        if (tipo == "") tipo = "No especificado";

        insertarGalaxia(id, codigo, nombre, atof(x.c_str()),
                        atof(y.c_str()), atof(z.c_str()), false,
                        tipo, descripcion);
    }

    archivo.close();
}


// Lee rutas.txt, relaciona las galaxias y reconstruye las rutas del grafo.
void cargarRutasArchivo() {
    ifstream archivo("rutas.txt");
    string linea;

    while (getline(archivo, linea)) {
        stringstream datos(linea);
        string id, origen, destino, costo, dirigida;

        getline(datos, id, '|');
        getline(datos, origen, '|');
        getline(datos, destino, '|');
        getline(datos, costo, '|');
        getline(datos, dirigida, '|');

        insertarRuta(id, origen, destino, atof(costo.c_str()),
                     atoi(dirigida.c_str()) == 1, false);
    }

    archivo.close();
}


// Lee naves.txt y reconstruye la lista enlazada de naves.
void cargarNavesArchivo() {
    ifstream archivo("naves.txt");
    string linea;

    while (getline(archivo, linea)) {
        stringstream datos(linea);
        string id, codigo, nombre;

        getline(datos, id, '|');
        getline(datos, codigo, '|');
        getline(datos, nombre, '|');

        insertarNave(id, codigo, nombre, false);
    }

    archivo.close();
}


// Lee historial.txt y reconstruye los viajes junto con sus pasos de ruta.
void cargarHistorialArchivo() {
    ifstream archivo("historial.txt");
    string linea;

    while (getline(archivo, linea)) {
        stringstream datos(linea);
        string id, nave, origen, destino, costo, fecha, rutas;

        getline(datos, id, '|');
        getline(datos, nave, '|');
        getline(datos, origen, '|');
        getline(datos, destino, '|');
        getline(datos, costo, '|');
        getline(datos, fecha, '|');
        getline(datos, rutas, '|');

        Viaje* viaje = insertarViaje(id, nave, origen, destino,
                                     atof(costo.c_str()), fecha, false);

        if (viaje != NULL) {
            stringstream listaRutas(rutas);
            string idRuta;

            while (getline(listaRutas, idRuta, ',')) {
                Ruta* ruta = buscarRuta(idRuta);
                if (ruta != NULL) agregarPasoViaje(viaje, ruta);
                else agregarPasoViajePorId(viaje, idRuta);
            }
        }
    }

    archivo.close();
}


// Comprueba que los cuatro archivos de respaldo requeridos se encuentren disponibles.
bool existenArchivosLocales() {
    ifstream galaxias("galaxias.txt");
    ifstream rutas("rutas.txt");
    ifstream naves("naves.txt");
    ifstream historial("historial.txt");

    bool existen = galaxias.good() && rutas.good() &&
                   naves.good() && historial.good();

    galaxias.close();
    rutas.close();
    naves.close();
    historial.close();
    return existen;
}


// Carga en el orden correcto todas las entidades almacenadas en archivos TXT.
void cargarDatosLocales() {
    if (!existenArchivosLocales()) {
        cout << "No existen archivos locales para cargar." << endl;
        return;
    }

    cargarGalaxiasArchivo();
    cargarRutasArchivo();
    cargarNavesArchivo();
    cargarHistorialArchivo();
    cout << "Datos cargados desde los archivos locales." << endl;
}

// ============================================================================
// CARGA DESDE JSON
// ============================================================================


// Interpreta el arreglo de nodos de la API e inserta sus galaxias en la lista enlazada.
void cargarGalaxiasJson(json lista) {
    for (int i = 0; i < (int)lista.size(); i++) {
        json dato = lista[i];
        if (!dato.is_object()) continue;

        string id = leerCampoTexto(dato, "id", "_id", "galaxiaId");
        if (id == "") id = leerCampoTexto(dato, "codigo", "code", "clave");

        string codigo = leerCampoTexto(dato, "codigo", "code", "clave");
        string nombre = leerCampoTexto(dato, "nombre", "name", "titulo");
        string tipo = leerCampoTexto(dato, "tipo", "type", "clase");
        string descripcion = leerCampoTexto(dato, "descripcion", "description", "detalle");
        double x = 0, y = 0, z = 0;

        if (dato.contains("coordenadas") && dato["coordenadas"].is_object()) {
            x = leerCampoNumero(dato["coordenadas"], "x", "X");
            y = leerCampoNumero(dato["coordenadas"], "y", "Y");
            z = leerCampoNumero(dato["coordenadas"], "z", "Z");
        } else {
            x = leerCampoNumero(dato, "x", "coordX", "coordenadaX");
            y = leerCampoNumero(dato, "y", "coordY", "coordenadaY");
            z = leerCampoNumero(dato, "z", "coordZ", "coordenadaZ");
        }

        if (id == "") continue;
        if (codigo == "") codigo = id;
        if (nombre == "") nombre = "Galaxia " + codigo;
        if (tipo == "") tipo = "No especificado";

        insertarGalaxia(id, codigo, nombre, x, y, z, false,
                        tipo, descripcion);
    }
}


// Interpreta las aristas de la API, valida sus referencias y reporta rutas insertadas u omitidas.
void cargarRutasJson(json lista, bool grafoDirigido) {
    int insertadas = 0;
    int omitidas = 0;

    vector<string> llavesId = {
        "id", "_id", "rutaId", "idRuta", "codigo", "code", "clave"
    };
    vector<string> llavesOrigen = {
        "origen", "origenId", "idOrigen", "galaxiaOrigen", "galaxiaOrigenId",
        "idGalaxiaOrigen", "source", "sourceId", "from", "fromId",
        "inicio", "nodoOrigen", "verticeOrigen", "nodoA", "verticeA",
        "galaxiaA", "galaxia1"
    };
    vector<string> llavesDestino = {
        "destino", "destinoId", "idDestino", "galaxiaDestino", "galaxiaDestinoId",
        "idGalaxiaDestino", "target", "targetId", "to", "toId",
        "fin", "nodoDestino", "verticeDestino", "nodoB", "verticeB",
        "galaxiaB", "galaxia2"
    };
    vector<string> llavesCosto = {
        "costo", "peso", "tiempo", "duracion", "weight", "cost",
        "costoTiempo", "tiempoViaje", "duracionViaje", "minutos", "distancia"
    };
    vector<string> llavesDirigida = {
        "dirigida", "esDirigida", "directed", "esOrientada", "orientada"
    };

    for (int i = 0; i < (int)lista.size(); i++) {
        json dato = lista[i];
        if (!dato.is_object()) {
            omitidas++;
            continue;
        }

        string id = buscarTextoApi(dato, llavesId);
        string origenRecibido = buscarTextoApi(dato, llavesOrigen);
        string destinoRecibido = buscarTextoApi(dato, llavesDestino);
        double costo = buscarNumeroApi(dato, llavesCosto);

        // La API define la orientacion general en meta.es_dirigido.
        // Ese valor se usa por defecto para todas las aristas. Si una arista
        // incluye su propia propiedad de direccion, esta tiene prioridad.
        bool dirigida = grafoDirigido;
        bool encontroDirigida = false;
        bool direccionParticular = buscarBoolApi(dato, llavesDirigida, encontroDirigida);

        if (encontroDirigida) {
            dirigida = direccionParticular;
        }

        Galaxia* origen = buscarGalaxiaFlexible(origenRecibido);
        Galaxia* destino = buscarGalaxiaFlexible(destinoRecibido);

        if (id == "" && origen != NULL && destino != NULL) {
            id = "API-Rut-" + to_string(i + 1);
        }

        if (id == "" || origen == NULL || destino == NULL || costo <= 0) {
            omitidas++;
            cout << "Ruta API omitida en posicion " << i << "." << endl;
            cout << "  ID: " << id
                 << " | Origen recibido: " << origenRecibido
                 << " | Destino recibido: " << destinoRecibido
                 << " | Costo: " << costo << endl;
            cout << "  Posible causa: faltan referencias validas de nave, galaxias o costo." << endl;
            cout << "  JSON: " << dato.dump() << endl;
            continue;
        }

        if (insertarRuta(id, origen->id, destino->id, costo, dirigida, false)) {
            insertadas++;
        } else {
            omitidas++;
        }
    }

    cout << "Rutas de API insertadas: " << insertadas
         << " | Omitidas: " << omitidas << endl;
}


// Interpreta el arreglo de naves de la API e inserta los registros validos.
void cargarNavesJson(json lista) {
    for (int i = 0; i < (int)lista.size(); i++) {
        json dato = lista[i];
        if (!dato.is_object()) continue;

        string id = leerCampoTexto(dato, "id", "_id", "naveId");
        if (id == "") id = leerCampoTexto(dato, "codigo", "code", "clave");

        string codigo = leerCampoTexto(dato, "codigo", "code", "clave");
        string nombre = leerCampoTexto(dato, "nombre", "name", "modelo");

        if (id == "") continue;
        if (codigo == "") codigo = id;
        if (nombre == "") nombre = "Nave " + codigo;
        insertarNave(id, codigo, nombre, false);
    }
}

// Busca las rutas mencionadas dentro de un registro del historial de la API.
// La API puede enviar una sola ruta o un arreglo de rutas y no siempre incluye
// directamente el origen, el destino o el costo del viaje.
vector<Ruta*> obtenerRutasHistorialApi(json dato) {
    vector<Ruta*> rutasEncontradas;

    if (!dato.is_object()) return rutasEncontradas;

    vector<string> llavesRuta = {
        "ruta", "rutaId", "idRuta", "codigoRuta",
        "route", "routeId", "arista", "aristaId", "idArista"
    };

    vector<string> llavesRutas = {
        "rutas", "rutasUsadas", "rutasVisitadas", "rutaIds",
        "recorrido", "trayecto", "routes", "routeIds", "aristas"
    };

    // Revisa todas las propiedades del objeto para encontrar una ruta o una lista.
    for (json::iterator it = dato.begin(); it != dato.end(); ++it) {
        if (claveCoincideApi(it.key(), llavesRuta)) {
            if (it.value().is_array()) {
                for (int i = 0; i < (int)it.value().size(); i++) {
                    string idRuta = extraerReferenciaApi(it.value()[i]);
                    Ruta* ruta = buscarRuta(idRuta);

                    if (ruta != NULL) {
                        bool repetida = false;
                        for (int j = 0; j < (int)rutasEncontradas.size(); j++) {
                            if (rutasEncontradas[j] == ruta) repetida = true;
                        }
                        if (!repetida) rutasEncontradas.push_back(ruta);
                    }
                }
            } else {
                string idRuta = extraerReferenciaApi(it.value());
                Ruta* ruta = buscarRuta(idRuta);
                if (ruta != NULL) rutasEncontradas.push_back(ruta);
            }
        }

        if (claveCoincideApi(it.key(), llavesRutas) && it.value().is_array()) {
            for (int i = 0; i < (int)it.value().size(); i++) {
                string idRuta = extraerReferenciaApi(it.value()[i]);
                Ruta* ruta = buscarRuta(idRuta);

                if (ruta == NULL && it.value()[i].is_object()) {
                    string origenRecibido = buscarTextoApi(it.value()[i], {
                        "origen", "origenId", "idOrigen", "source", "from"
                    });
                    string destinoRecibido = buscarTextoApi(it.value()[i], {
                        "destino", "destinoId", "idDestino", "target", "to"
                    });

                    Galaxia* origen = buscarGalaxiaFlexible(origenRecibido);
                    Galaxia* destino = buscarGalaxiaFlexible(destinoRecibido);

                    if (origen != NULL && destino != NULL) {
                        ruta = buscarRutaEntre(origen->id, destino->id);
                    }
                }

                if (ruta != NULL) {
                    bool repetida = false;
                    for (int j = 0; j < (int)rutasEncontradas.size(); j++) {
                        if (rutasEncontradas[j] == ruta) repetida = true;
                    }
                    if (!repetida) rutasEncontradas.push_back(ruta);
                }
            }
        }
    }

    // Algunos servicios envuelven el registro dentro de data, registro o viaje.
    vector<string> contenedores = {
        "data", "registro", "viaje", "historial", "resultado"
    };

    for (json::iterator it = dato.begin(); it != dato.end(); ++it) {
        if (it.value().is_object() && claveCoincideApi(it.key(), contenedores)) {
            vector<Ruta*> internas = obtenerRutasHistorialApi(it.value());

            for (int i = 0; i < (int)internas.size(); i++) {
                bool repetida = false;
                for (int j = 0; j < (int)rutasEncontradas.size(); j++) {
                    if (rutasEncontradas[j] == internas[i]) repetida = true;
                }
                if (!repetida) rutasEncontradas.push_back(internas[i]);
            }
        }
    }

    return rutasEncontradas;
}

// Determina origen, destino y costo a partir de las rutas del historial.
// Esto permite cargar registros de la API que solo contienen naveId y rutaId.
bool obtenerDatosDesdeRutas(vector<Ruta*> rutas, Galaxia*& origen,
                            Galaxia*& destino, double& costo) {
    if (rutas.size() == 0) return false;

    costo = 0;
    for (int i = 0; i < (int)rutas.size(); i++) {
        if (rutas[i] != NULL) costo += rutas[i]->costo;
    }

    if (rutas.size() == 1) {
        origen = rutas[0]->origen;
        destino = rutas[0]->destino;
        return origen != NULL && destino != NULL && costo > 0;
    }

    Ruta* primera = rutas[0];
    Ruta* segunda = rutas[1];

    // Se determina en qué sentido debe recorrerse la primera ruta para que
    // conecte con la segunda.
    if (primera->destino == segunda->origen ||
        primera->destino == segunda->destino) {
        origen = primera->origen;
        destino = primera->destino;
    } else if (primera->origen == segunda->origen ||
               primera->origen == segunda->destino) {
        origen = primera->destino;
        destino = primera->origen;
    } else {
        // Si las rutas no vienen encadenadas, al menos se conserva el orden
        // original recibido por la API.
        origen = primera->origen;
        destino = primera->destino;
    }

    Galaxia* actual = destino;

    for (int i = 1; i < (int)rutas.size(); i++) {
        Ruta* ruta = rutas[i];

        if (ruta->origen == actual) {
            actual = ruta->destino;
        } else if (!ruta->dirigida && ruta->destino == actual) {
            actual = ruta->origen;
        } else if (ruta->destino == actual) {
            // Se admite el sentido contrario únicamente para poder reconstruir
            // historiales antiguos que no guardaron la orientación.
            actual = ruta->origen;
        } else {
            // La secuencia no es continua; se usa el destino de la última ruta.
            actual = ruta->destino;
        }
    }

    destino = actual;
    return origen != NULL && destino != NULL && origen != destino && costo > 0;
}


// Agrega a un viaje las rutas indicadas dentro de una respuesta JSON del historial.
void agregarRutasJsonAViaje(Viaje* viaje, json dato) {
    if (viaje == NULL) return;

    vector<Ruta*> rutas = obtenerRutasHistorialApi(dato);

    for (int i = 0; i < (int)rutas.size(); i++) {
        agregarPasoViaje(viaje, rutas[i]);
    }
}


// Interpreta los viajes de la API, resuelve sus asociaciones y reporta registros insertados u omitidos.
void cargarHistorialJson(json lista) {
    int insertados = 0;
    int omitidos = 0;

    for (int i = 0; i < (int)lista.size(); i++) {
        json dato = lista[i];
        if (!dato.is_object()) {
            omitidos++;
            continue;
        }

        string id = buscarTextoApi(dato, {
            "id", "_id", "viajeId", "idViaje", "historialId",
            "idHistorial", "codigo", "clave"
        });
        string naveRecibida = buscarTextoApi(dato, {
            "nave", "naveId", "idNave", "naveCodigo", "codigoNave",
            "ship", "shipId"
        });
        string origenRecibido = buscarTextoApi(dato, {
            "origen", "origenId", "idOrigen", "galaxiaOrigen",
            "galaxiaOrigenId", "origenNombre", "origen_nombre",
            "source", "sourceId", "from", "inicio"
        });
        string destinoRecibido = buscarTextoApi(dato, {
            "destino", "destinoId", "idDestino", "galaxiaDestino",
            "galaxiaDestinoId", "destinoNombre", "destino_nombre",
            "target", "targetId", "to", "fin"
        });
        string fecha = buscarTextoApi(dato, {
            "fecha", "date", "fechaViaje", "createdAt", "fechaRegistro"
        });
        double costo = buscarNumeroApi(dato, {
            "costo", "costoReal", "costo_real", "tiempo", "duracion",
            "costoTotal", "total", "weight", "cost"
        });

        Nave* nave = buscarNaveFlexible(naveRecibida);
        Galaxia* origen = buscarGalaxiaFlexible(origenRecibido);
        Galaxia* destino = buscarGalaxiaFlexible(destinoRecibido);
        vector<Ruta*> rutasHistorial = obtenerRutasHistorialApi(dato);
        string idRutaHistorica = buscarTextoApi(dato, {
            "ruta", "rutaId", "idRuta", "ruta_id",
            "route", "routeId", "arista", "aristaId"
        });

        if (id == "") id = "API-Via-" + to_string(i + 1);
        if (fecha == "") fecha = "No indicada";

        // La forma usada por la API puede ser: naveId + rutaId.
        // En ese caso se obtienen origen, destino y costo desde la ruta.
        if ((origen == NULL || destino == NULL || costo <= 0) &&
            rutasHistorial.size() > 0) {
            Galaxia* origenRuta = NULL;
            Galaxia* destinoRuta = NULL;
            double costoRutas = 0;

            if (obtenerDatosDesdeRutas(rutasHistorial, origenRuta,
                                       destinoRuta, costoRutas)) {
                if (origen == NULL) origen = origenRuta;
                if (destino == NULL) destino = destinoRuta;
                if (costo <= 0) costo = costoRutas;
            }
        }

        // Ultimo respaldo: calcular el costo mediante Dijkstra cuando la API
        // sí suministra los extremos, pero no el costo.
        ResultadoCamino camino;
        camino.encontrado = false;

        if (costo <= 0 && origen != NULL && destino != NULL) {
            camino = dijkstra(origen->id, destino->id);
            if (camino.encontrado) costo = camino.costoTotal;
        }

        if (nave == NULL || origen == NULL || destino == NULL || costo <= 0) {
            omitidos++;
            cout << "Viaje API omitido en posicion " << i << "." << endl;
            cout << "  Nave recibida: " << naveRecibida
                 << " | Origen: " << origenRecibido
                 << " | Destino: " << destinoRecibido
                 << " | Costo: " << costo
                 << " | Rutas reconocidas: " << rutasHistorial.size() << endl;
            cout << "  JSON: " << dato.dump() << endl;
            continue;
        }

        Viaje* viaje = insertarViaje(id, nave->id, origen->id, destino->id,
                                     costo, fecha, false);

        if (viaje != NULL) {
            for (int j = 0; j < (int)rutasHistorial.size(); j++) {
                agregarPasoViaje(viaje, rutasHistorial[j]);
            }

            // El historial puede referirse a una ruta que ya no esta activa y,
            // por eso, no aparece dentro de las 17 aristas de /grafo. Se conserva
            // su identificador sin incorporarla a Dijkstra ni a Kruskal.
            if (viaje->rutasUsadas == NULL && idRutaHistorica != "") {
                agregarPasoViajePorId(viaje, idRutaHistorica);
            }

            // Si la API no especifica ninguna ruta, se utiliza Dijkstra.
            if (viaje->rutasUsadas == NULL) {
                if (!camino.encontrado) camino = dijkstra(origen->id, destino->id);

                if (camino.encontrado) {
                    for (int j = 0; j < (int)camino.rutas.size(); j++) {
                        agregarPasoViaje(viaje, camino.rutas[j]);
                    }
                }
            }

            insertados++;
        } else {
            omitidos++;
        }
    }

    cout << "Viajes de API insertados: " << insertados
         << " | Omitidos: " << omitidos << endl;
}


// Carga el orden aleatorio de rutas recibido desde el endpoint destinado a Kruskal.
void cargarOrdenKruskalJson(json lista) {
    ordenKruskal.clear();

    for (int i = 0; i < (int)lista.size(); i++) {
        json dato = lista[i];
        string id = "";

        if (dato.is_string() || dato.is_number()) {
            id = convertirJsonATexto(dato);
        } else if (dato.is_object()) {
            id = leerCampoTexto(dato, "id", "_id", "rutaId");
            if (id == "") id = leerCampoTexto(dato, "codigo", "code");
        }

        Ruta* ruta = buscarRuta(id);

        if (ruta == NULL && dato.is_object()) {
            string origen = leerCampoTexto(dato, "origenId", "origen", "desde");
            if (origen == "") origen = leerCampoTexto(dato, "source", "sourceId", "idOrigen");

            string destino = leerCampoTexto(dato, "destinoId", "destino", "hasta");
            if (destino == "") destino = leerCampoTexto(dato, "target", "targetId", "idDestino");

            ruta = buscarRutaEntre(origen, destino);
        }

        if (ruta != NULL) {
            bool repetida = false;
            for (int j = 0; j < (int)ordenKruskal.size(); j++) {
                if (ordenKruskal[j] == ruta) repetida = true;
            }
            if (!repetida) ordenKruskal.push_back(ruta);
        }
    }
}


// Coordina la carga automatica desde la API, usa endpoints alternativos y muestra un resumen detallado.
bool cargarDatosApi() {
    cout << "Cargando informacion desde la API..." << endl;

    json galaxias = json::array();
    json rutas = json::array();
    json naves = json::array();
    json historial = json::array();
    bool grafoDirigido = false;

    // Primero se intenta el endpoint de carga completa solicitado en la consigna.
    json grafo = obtenerJson("/grafo");

    // Se conserva una copia para revisar facilmente la estructura real de la API.
    if (!grafo.is_null() && !grafo.empty()) {
        ofstream archivoApi("respuesta_api_grafo.json");
        if (archivoApi.is_open()) {
            archivoApi << grafo.dump(4);
            archivoApi.close();
        }
    }

    if (!grafo.is_null() && !grafo.empty()) {
        if (grafo.contains("meta") && grafo["meta"].is_object() &&
            grafo["meta"].contains("es_dirigido")) {
            grafoDirigido = leerCampoBool(grafo["meta"], "es_dirigido");
        }

        galaxias = obtenerArreglo(grafo, "galaxias");
        if (galaxias.size() == 0) galaxias = obtenerArreglo(grafo, "nodos");
        if (galaxias.size() == 0) galaxias = obtenerArreglo(grafo, "nodes");
        if (galaxias.size() == 0) galaxias = obtenerArreglo(grafo, "vertices");

        rutas = obtenerArreglo(grafo, "rutas");
        if (rutas.size() == 0) rutas = obtenerArreglo(grafo, "aristas");
        if (rutas.size() == 0) rutas = obtenerArreglo(grafo, "edges");
        if (rutas.size() == 0) rutas = obtenerArreglo(grafo, "conexiones");

        naves = obtenerArreglo(grafo, "naves");
        historial = obtenerArreglo(grafo, "historial");
        if (historial.size() == 0) historial = obtenerArreglo(grafo, "viajes");
    }

    // Si /grafo falla o no contiene alguna entidad, se usa el endpoint individual.
    if (galaxias.size() == 0) {
        json datos = obtenerJson("/galaxias");
        galaxias = obtenerArreglo(datos, "galaxias");
        if (galaxias.size() == 0) galaxias = obtenerArreglo(datos, "nodos");
    }

    if (rutas.size() == 0) {
        json datos = obtenerJson("/rutas");

        if (datos.is_object() && datos.contains("meta") &&
            datos["meta"].is_object() &&
            datos["meta"].contains("es_dirigido")) {
            grafoDirigido = leerCampoBool(datos["meta"], "es_dirigido");
        }

        rutas = obtenerArreglo(datos, "rutas");
        if (rutas.size() == 0) rutas = obtenerArreglo(datos, "aristas");
    }

    if (naves.size() == 0) {
        json datos = obtenerJson("/naves");
        naves = obtenerArreglo(datos, "naves");
    }

    if (historial.size() == 0) {
        json datos = obtenerJson("/historial");
        historial = obtenerArreglo(datos, "historial");
        if (historial.size() == 0) historial = obtenerArreglo(datos, "viajes");
    }

    cout << "Registros recibidos: "
         << galaxias.size() << " galaxias, "
         << rutas.size() << " rutas, "
         << naves.size() << " naves y "
         << historial.size() << " viajes." << endl;
    cout << "Tipo de grafo recibido: "
         << (grafoDirigido ? "Dirigido" : "No dirigido") << endl;

    // Las rutas dependen de las galaxias y los viajes dependen de todo lo anterior.
    cargarGalaxiasJson(galaxias);
    cargarRutasJson(rutas, grafoDirigido);
    cargarNavesJson(naves);
    cargarHistorialJson(historial);

    json datosKruskal = obtenerJson("/grafo/kruskal");
    json listaKruskal = obtenerArreglo(datosKruskal, "rutas");
    if (listaKruskal.size() == 0) listaKruskal = obtenerArreglo(datosKruskal, "aristas");
    if (listaKruskal.size() == 0) listaKruskal = obtenerArreglo(datosKruskal, "edges");
    cargarOrdenKruskalJson(listaKruskal);

    cout << "Datos insertados en memoria: "
         << contarGalaxias() << " galaxias, "
         << contarRutas() << " rutas, "
         << contarNaves() << " naves y "
         << contarViajes() << " viajes." << endl;

    if (primeraGalaxia == NULL || primeraRuta == NULL) {
        cout << "La API no proporciono un grafo completo." << endl;
        return false;
    }

    if (ordenKruskal.size() == 0) {
        cout << "Aviso: no se recibio el orden de Kruskal. "
             << "Se utilizara el orden aleatorio local cuando se solicite." << endl;
    }

    guardarDatosLocales();

    if (historial.size() > 0 && contarViajes() < (int)historial.size()) {
        cout << "Carga automatica finalizada con advertencias: "
             << contarViajes() << " de " << historial.size()
             << " registros del historial fueron cargados." << endl;
    } else {
        cout << "Carga automatica finalizada correctamente." << endl;
    }

    return true;
}

// ============================================================================
// REPORTES
// ============================================================================


// Calcula una ruta minima y agrega sus resultados al archivo reporte_rutas_cortas.txt.
void reporteRutaCorta() {
    string origen = leerTexto("ID o codigo de galaxia origen: ");
    string destino = leerTexto("ID o codigo de galaxia destino: ");
    ResultadoCamino resultado = dijkstra(origen, destino);

    if (!resultado.encontrado) return;

    ofstream archivo("reporte_rutas_cortas.txt", ios::app);
    archivo << "Ruta: ";

    for (int i = 0; i < (int)resultado.galaxias.size(); i++) {
        archivo << resultado.galaxias[i]->nombre;
        if (i < (int)resultado.galaxias.size() - 1) archivo << " -> ";
    }

    archivo << " | Rutas: ";
    for (int i = 0; i < (int)resultado.rutas.size(); i++) {
        archivo << resultado.rutas[i]->id;
        if (i < (int)resultado.rutas.size() - 1) archivo << ", ";
    }
    archivo << " | Costo total: " << resultado.costoTotal << endl;
    archivo.close();
    cout << "Reporte generado: reporte_rutas_cortas.txt" << endl;
}


// Genera el archivo del arbol de expansion obtenido mediante Kruskal modificado.
void reporteArbolExpansion() {
    vector<Ruta*> arbol = kruskalModificado();
    ofstream archivo("reporte_arbol_expansion.txt");
    double total = 0;

    archivo << "ARBOL DE EXPANSION - KRUSKAL MODIFICADO" << endl << endl;

    for (int i = 0; i < (int)arbol.size(); i++) {
        archivo << arbol[i]->origen->nombre << " -- "
                << arbol[i]->destino->nombre
                << " | Costo: " << arbol[i]->costo << endl;
        total += arbol[i]->costo;
    }

    archivo << endl << "Costo total: " << total << endl;
    archivo.close();
    cout << "Reporte generado: reporte_arbol_expansion.txt" << endl;
}


// Cuenta y retorna la cantidad de galaxias almacenadas en la lista enlazada.
int contarGalaxias() {
    int cantidad = 0;
    Galaxia* actual = primeraGalaxia;
    while (actual != NULL) {
        cantidad++;
        actual = actual->sigG;
    }
    return cantidad;
}


// Cuenta y retorna la cantidad de rutas activas registradas.
int contarRutas() {
    int cantidad = 0;
    Ruta* actual = primeraRuta;
    while (actual != NULL) {
        cantidad++;
        actual = actual->sigR;
    }
    return cantidad;
}


// Cuenta y retorna la cantidad de naves almacenadas.
int contarNaves() {
    int cantidad = 0;
    Nave* actual = primeraNave;
    while (actual != NULL) {
        cantidad++;
        actual = actual->sigN;
    }
    return cantidad;
}


// Cuenta y retorna la cantidad de registros existentes en el historial.
int contarViajes() {
    int cantidad = 0;
    Viaje* actual = primerViaje;
    while (actual != NULL) {
        cantidad++;
        actual = actual->sigV;
    }
    return cantidad;
}


// Cuenta las conexiones salientes presentes en la lista de adyacencia de una galaxia.
int conectividadGalaxia(Galaxia* galaxia) {
    int cantidad = 0;
    Arco* arco = galaxia->subListaArcos;
    while (arco != NULL) {
        cantidad++;
        arco = arco->sigA;
    }
    return cantidad;
}


// Genera estadisticas de entidades, conectividad y comparacion entre rutas originales y optimizadas.
void reporteEstadisticas() {
    ofstream archivo("reporte_estadisticas.txt");
    vector<Ruta*> arbol = kruskalModificado();
    double costoOriginal = 0;
    double costoOptimizado = 0;
    int mayorConectividad = -1;

    Ruta* ruta = primeraRuta;
    while (ruta != NULL) {
        costoOriginal += ruta->costo;
        ruta = ruta->sigR;
    }

    for (int i = 0; i < (int)arbol.size(); i++) {
        costoOptimizado += arbol[i]->costo;
    }

    Galaxia* galaxia = primeraGalaxia;
    while (galaxia != NULL) {
        int conexiones = conectividadGalaxia(galaxia);
        if (conexiones > mayorConectividad) mayorConectividad = conexiones;
        galaxia = galaxia->sigG;
    }

    archivo << "INFORME DE ESTADISTICAS" << endl << endl;
    archivo << "Cantidad de galaxias: " << contarGalaxias() << endl;
    archivo << "Cantidad de rutas originales: " << contarRutas() << endl;
    archivo << "Cantidad de naves: " << contarNaves() << endl;
    archivo << "Cantidad de viajes: " << contarViajes() << endl;
    archivo << "Rutas minimas calculadas durante la ejecucion: "
            << cantidadRutasCalculadas << endl << endl;

    archivo << "Galaxias con mayor conectividad:" << endl;
    galaxia = primeraGalaxia;
    while (galaxia != NULL) {
        if (conectividadGalaxia(galaxia) == mayorConectividad) {
            archivo << "- " << galaxia->nombre << ": "
                    << mayorConectividad << " conexiones" << endl;
        }
        galaxia = galaxia->sigG;
    }

    archivo << endl << "Comparacion de costos:" << endl;
    archivo << "Costo de todas las rutas originales: " << costoOriginal << endl;
    archivo << "Costo del arbol optimizado: " << costoOptimizado << endl;
    archivo << "Cantidad de rutas del arbol: " << arbol.size() << endl;

    double ahorro = costoOriginal - costoOptimizado;
    archivo << "Ahorro absoluto: " << ahorro << endl;
    if (costoOriginal > 0) {
        archivo << "Reduccion porcentual: "
                << (ahorro * 100.0 / costoOriginal) << "%" << endl;
    }

    if (contarGalaxias() > 0 && (int)arbol.size() != contarGalaxias() - 1) {
        archivo << "Aviso: el grafo no quedo completamente conectado." << endl;
    }

    archivo.close();
    cout << "Reporte generado: reporte_estadisticas.txt" << endl;
}

// ============================================================================
// LIBERACION DE MEMORIA
// ============================================================================


// Libera todas las listas dinamicas y estructuras auxiliares antes de finalizar el programa.
void liberarMemoria() {
    while (primerViaje != NULL) {
        Viaje* borrar = primerViaje;
        primerViaje = primerViaje->sigV;
        liberarPasos(borrar->rutasUsadas);
        delete borrar;
    }

    while (primeraRuta != NULL) {
        Ruta* borrar = primeraRuta;
        primeraRuta = primeraRuta->sigR;
        delete borrar;
    }

    while (primeraNave != NULL) {
        Nave* borrar = primeraNave;
        primeraNave = primeraNave->sigN;
        delete borrar;
    }

    while (primeraGalaxia != NULL) {
        Galaxia* borrar = primeraGalaxia;
        primeraGalaxia = primeraGalaxia->sigG;
        liberarArcos(borrar);
        delete borrar;
    }

    ordenKruskal.clear();
}

// ============================================================================
// MENUS CRUD
// ============================================================================


// Administra las opciones CRUD y consultas disponibles para la entidad galaxia.
void menuGalaxias() {
    int opcion;

    do {
        cout << "\n------------- MENU GALAXIAS -------------" << endl;
        cout << "1. Mostrar galaxias" << endl;
        cout << "2. Insertar galaxia" << endl;
        cout << "3. Buscar galaxia" << endl;
        cout << "4. Modificar galaxia" << endl;
        cout << "5. Eliminar galaxia" << endl;
        cout << "0. Volver" << endl;
        opcion = leerEntero("Opcion: ");

        switch (opcion) {
            case 1:
                mostrarGalaxias();
                break;
            case 2: {
                string id = leerTexto("ID: ");
                string codigo = leerTexto("Codigo: ");
                string nombre = leerTexto("Nombre: ");
                string tipo = leerTexto("Tipo: ");
                string descripcion = leerTexto("Descripcion: ");
                double x = leerDecimal("Coordenada X: ");
                double y = leerDecimal("Coordenada Y: ");
                double z = leerDecimal("Coordenada Z: ");
                if (insertarGalaxia(id, codigo, nombre, x, y, z, true,
                                    tipo, descripcion)) {
                    guardarDatosLocales();
                }
                break;
            }
            case 3: {
                Galaxia* galaxia = buscarGalaxia(leerTexto("ID, codigo o nombre: "));
                if (galaxia == NULL) cout << "Galaxia no encontrada." << endl;
                else {
                    cout << "ID: " << galaxia->id << endl;
                    cout << "Codigo: " << galaxia->codigo << endl;
                    cout << "Nombre: " << galaxia->nombre << endl;
                    cout << "Tipo: " << galaxia->tipo << endl;
                    cout << "Descripcion: "
                         << (galaxia->descripcion == "" ? "No disponible" : galaxia->descripcion)
                         << endl;
                    cout << "Coordenadas: (" << galaxia->x << ", "
                         << galaxia->y << ", " << galaxia->z << ")" << endl;
                }
                break;
            }
            case 4:
                modificarGalaxia();
                break;
            case 5:
                eliminarGalaxia();
                break;
            case 0:
                break;
            default:
                cout << "Opcion invalida." << endl;
        }
    } while (opcion != 0);
}


// Administra las opciones CRUD y consultas disponibles para las rutas.
void menuRutas() {
    int opcion;

    do {
        cout << "\n--------------- MENU RUTAS ---------------" << endl;
        cout << "1. Mostrar rutas" << endl;
        cout << "2. Insertar ruta" << endl;
        cout << "3. Buscar ruta" << endl;
        cout << "4. Modificar ruta" << endl;
        cout << "5. Eliminar ruta" << endl;
        cout << "0. Volver" << endl;
        opcion = leerEntero("Opcion: ");

        switch (opcion) {
            case 1:
                mostrarRutas();
                break;
            case 2: {
                string id = leerTexto("ID de ruta: ");
                string origen = leerTexto("ID, codigo o nombre de galaxia origen: ");
                string destino = leerTexto("ID, codigo o nombre de galaxia destino: ");
                double costo = leerDecimal("Costo: ");
                int tipo;
                do {
                    tipo = leerEntero("Tipo (1 dirigida, 2 no dirigida): ");
                    if (tipo != 1 && tipo != 2) cout << "Digite 1 o 2." << endl;
                } while (tipo != 1 && tipo != 2);
                if (insertarRuta(id, origen, destino, costo, tipo == 1)) {
                    ordenKruskal.clear();
                    guardarDatosLocales();
                }
                break;
            }
            case 3: {
                Ruta* ruta = buscarRuta(leerTexto("ID de ruta: "));
                if (ruta == NULL) cout << "Ruta no encontrada." << endl;
                else cout << ruta->origen->nombre << " -> "
                          << ruta->destino->nombre << " | Costo: "
                          << ruta->costo << endl;
                break;
            }
            case 4:
                modificarRuta();
                break;
            case 5:
                eliminarRuta();
                break;
            case 0:
                break;
            default:
                cout << "Opcion invalida." << endl;
        }
    } while (opcion != 0);
}


// Administra las opciones CRUD, busqueda e historial disponibles para las naves.
void menuNaves() {
    int opcion;

    do {
        cout << "\n--------------- MENU NAVES ---------------" << endl;
        cout << "1. Mostrar naves" << endl;
        cout << "2. Insertar nave" << endl;
        cout << "3. Buscar nave" << endl;
        cout << "4. Modificar nave" << endl;
        cout << "5. Eliminar nave" << endl;
        cout << "6. Historial de una nave" << endl;
        cout << "0. Volver" << endl;
        opcion = leerEntero("Opcion: ");

        switch (opcion) {
            case 1:
                mostrarNaves();
                break;
            case 2: {
                string id = leerTexto("ID: ");
                string codigo = leerTexto("Codigo: ");
                string nombre = leerTexto("Nombre: ");
                if (insertarNave(id, codigo, nombre)) guardarDatosLocales();
                break;
            }
            case 3: {
                Nave* nave = buscarNave(leerTexto("ID, codigo o nombre: "));
                if (nave == NULL) cout << "Nave no encontrada." << endl;
                else cout << nave->codigo << " - " << nave->nombre << endl;
                break;
            }
            case 4:
                modificarNave();
                break;
            case 5:
                eliminarNave();
                break;
            case 6:
                mostrarHistorialNave(leerTexto("ID, codigo o nombre de nave: "));
                break;
            case 0:
                break;
            default:
                cout << "Opcion invalida." << endl;
        }
    } while (opcion != 0);
}


// Administra el registro, consulta, modificacion y eliminacion de viajes.
void menuHistorial() {
    int opcion;

    do {
        cout << "\n------------- MENU HISTORIAL -------------" << endl;
        cout << "1. Mostrar todos los viajes" << endl;
        cout << "2. Registrar viaje por ruta minima" << endl;
        cout << "3. Buscar viaje" << endl;
        cout << "4. Modificar viaje" << endl;
        cout << "5. Eliminar viaje" << endl;
        cout << "0. Volver" << endl;
        opcion = leerEntero("Opcion: ");

        switch (opcion) {
            case 1:
                mostrarHistorial();
                break;
            case 2:
                registrarViaje();
                break;
            case 3: {
                Viaje* viaje = buscarViaje(leerTexto("ID del viaje: "));
                if (viaje == NULL) cout << "Viaje no encontrado." << endl;
                else mostrarViaje(viaje);
                break;
            }
            case 4:
                modificarViaje();
                break;
            case 5:
                eliminarViaje();
                break;
            case 0:
                break;
            default:
                cout << "Opcion invalida." << endl;
        }
    } while (opcion != 0);
}


// Agrupa las consultas principales relacionadas con recorridos y algoritmos del grafo.
void menuConsultas() {
    int opcion;

    do {
        cout << "\n-------------- CONSULTAS ----------------" << endl;
        cout << "1. Mostrar rutas desde una galaxia" << endl;
        cout << "2. Ruta de menor costo entre galaxias" << endl;
        cout << "3. Arbol resultante de Kruskal" << endl;
        cout << "4. Historial de viajes por nave" << endl;
        cout << "0. Volver" << endl;
        opcion = leerEntero("Opcion: ");

        switch (opcion) {
            case 1:
                mostrarRutasDesdeGalaxia(leerTexto("ID o codigo de galaxia: "));
                break;
            case 2:
                consultarRutaMinima();
                break;
            case 3:
                mostrarKruskal();
                break;
            case 4:
                mostrarHistorialNave(leerTexto("ID, codigo o nombre de nave: "));
                break;
            case 0:
                break;
            default:
                cout << "Opcion invalida." << endl;
        }
    } while (opcion != 0);
}


// Permite generar los archivos de rutas cortas, arbol de expansion y estadisticas.
void menuReportes() {
    int opcion;

    do {
        cout << "\n--------------- REPORTES ----------------" << endl;
        cout << "1. Archivo de ruta mas corta" << endl;
        cout << "2. Archivo del arbol de expansion" << endl;
        cout << "3. Informe de estadisticas" << endl;
        cout << "0. Volver" << endl;
        opcion = leerEntero("Opcion: ");

        switch (opcion) {
            case 1:
                reporteRutaCorta();
                break;
            case 2:
                reporteArbolExpansion();
                break;
            case 3:
                reporteEstadisticas();
                break;
            case 0:
                break;
            default:
                cout << "Opcion invalida." << endl;
        }
    } while (opcion != 0);
}


// Presenta la navegacion general y dirige al usuario hacia cada seccion del sistema.
void menuPrincipal() {
    int opcion;

    do {
        cout << "\n===========================================" << endl;
        cout << "       SISTEMA DE RUTAS GALACTICAS" << endl;
        cout << "===========================================" << endl;
        cout << "1. Galaxias" << endl;
        cout << "2. Rutas" << endl;
        cout << "3. Naves" << endl;
        cout << "4. Historial de viajes" << endl;
        cout << "5. Consultas del grafo" << endl;
        cout << "6. Reportes" << endl;
        cout << "7. Guardar datos en archivos" << endl;
        cout << "0. Salir" << endl;
        opcion = leerEntero("Opcion: ");

        switch (opcion) {
            case 1:
                menuGalaxias();
                break;
            case 2:
                menuRutas();
                break;
            case 3:
                menuNaves();
                break;
            case 4:
                menuHistorial();
                break;
            case 5:
                menuConsultas();
                break;
            case 6:
                menuReportes();
                break;
            case 7:
                guardarDatosLocales();
                cout << "Datos guardados correctamente." << endl;
                break;
            case 0:
                cout << "Saliendo del sistema..." << endl;
                break;
            default:
                cout << "Opcion invalida." << endl;
        }
    } while (opcion != 0);
}

// ============================================================================
// FUNCION PRINCIPAL
// ============================================================================


// Inicializa libcurl, carga los datos, ejecuta el menu principal y libera todos los recursos al terminar.
int main() {
    srand((unsigned int)time(NULL));

    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        cout << "No se pudo inicializar libcurl." << endl;
        return 1;
    }

    cout << "Version de libcurl: " << curl_version() << endl;

    // La carga desde la API es automatica, como solicita la rubrica.
    bool apiCargada = cargarDatosApi();

    if (!apiCargada) {
        // Para no mezclar registros de la API con archivos antiguos, los datos
        // locales solo se cargan cuando la API no logro insertar ningun dato.
        if (primeraGalaxia == NULL && primeraRuta == NULL &&
            primeraNave == NULL && primerViaje == NULL) {
            cout << "No fue posible cargar datos desde la API. "
                 << "Se intentaran usar archivos locales." << endl;
            cargarDatosLocales();
        } else {
            cout << "La API se cargo de forma parcial. "
                 << "Se conservaran solamente los registros recibidos de la API." << endl;
        }
    }

    menuPrincipal();

    guardarDatosLocales();
    liberarMemoria();
    curl_global_cleanup();
    return 0;
}

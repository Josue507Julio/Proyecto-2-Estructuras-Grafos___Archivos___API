/*
    PROYECTO PROGRAMADO: GRAFOS Y ARCHIVOS
    Sistema de rutas intergalacticas

    Fecha de inicio: 17/06/2026
    Ultima modificacion: 19/06/2026
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
        a. g++ -std=c++17 Proyecto-2-Estructuras-Grafos___Archivos___API.cpp -o Proyecto-2-Estructuras-Grafos___Archivos___API.exe -lcurl
        b. ./Proyecto-2-Estructuras-Grafos___Archivos___API.exe
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

const double INFINITO = 999999999.0;

const string URL_BASE = "https://galaxias-mock-api.onrender.com";

struct Ruta;
struct Galaxia;

struct Arco { Galaxia* destino; Ruta* ruta; Arco* siguiente; };
struct Galaxia {
    string id, codigo, nombre, tipo, descripcion;
    double x, y, z;
    Arco* arcos;
    Galaxia* siguiente;
};
struct Ruta {
    string id;
    Galaxia* origen;
    Galaxia* destino;
    double costo;
    bool dirigida;
    Ruta* siguiente;
};
struct Nave { string id, codigo, nombre; Nave* siguiente; };
struct PasoViaje { Ruta* ruta; string idRuta; PasoViaje* siguiente; };
struct Viaje {
    string id, fecha;
    Nave* nave;
    Galaxia* origen;
    Galaxia* destino;
    double costoTotal;
    PasoViaje* rutasUsadas;
    Viaje* siguiente;
};
struct ResultadoCamino {
    bool encontrado;
    double costoTotal;
    vector<Galaxia*> galaxias;
    vector<Ruta*> rutas;
};

Galaxia* primeraGalaxia = NULL;
Ruta* primeraRuta = NULL;
Nave* primeraNave = NULL;
Viaje* primerViaje = NULL;
vector<Ruta*> ordenKruskal;
int cantidadRutasCalculadas = 0;

string quitarEspaciosExtremos(string texto) {
    int inicio = 0;
    int fin = (int)texto.length() - 1;
    while (inicio <= fin && isspace((unsigned char)texto[inicio])) inicio++;
    while (fin >= inicio && isspace((unsigned char)texto[fin])) fin--;
    if (inicio > fin) return "";
    return texto.substr(inicio, fin - inicio + 1);
}

string normalizar(string texto) {
    texto = quitarEspaciosExtremos(texto);
    string resultado = "";
    for (int i = 0; i < (int)texto.length(); i++) {
        unsigned char c = (unsigned char)texto[i];
        if (isalnum(c)) resultado += (char)tolower(c);
    }
    return resultado;
}

void limpiarEntrada() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

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

string leerTexto(string mensaje) {
    string texto;
    cout << mensaje;
    getline(cin, texto);
    return quitarEspaciosExtremos(texto);
}

bool textoInvalido(string texto) {
    return texto == "" || texto.find('|') != string::npos || texto.find(',') != string::npos;
}

Galaxia* buscarGalaxiaPorId(string id) {
    Galaxia* actual = primeraGalaxia;
    while (actual != NULL) {
        if (normalizar(actual->id) == normalizar(id)) return actual;
        actual = actual->siguiente;
    }
    return NULL;
}

Galaxia* buscarGalaxiaPorCodigo(string codigo) {
    Galaxia* actual = primeraGalaxia;
    while (actual != NULL) {
        if (normalizar(actual->codigo) == normalizar(codigo)) return actual;
        actual = actual->siguiente;
    }
    return NULL;
}

Galaxia* buscarGalaxiaPorNombre(string nombre) {
    Galaxia* actual = primeraGalaxia;
    while (actual != NULL) {
        if (normalizar(actual->nombre) == normalizar(nombre)) return actual;
        actual = actual->siguiente;
    }
    return NULL;
}

Galaxia* buscarGalaxia(string valor) {
    Galaxia* encontrada = buscarGalaxiaPorId(valor);
    if (encontrada != NULL) return encontrada;
    encontrada = buscarGalaxiaPorCodigo(valor);
    if (encontrada != NULL) return encontrada;
    return buscarGalaxiaPorNombre(valor);
}

bool insertarGalaxia(string id, string codigo, string nombre,
                     string tipo, string descripcion,
                     double x, double y, double z) {
    if (textoInvalido(id) || textoInvalido(codigo) || textoInvalido(nombre)) {
        cout << "Error: existen datos vacios o caracteres no permitidos." << endl;
        return false;
    }
    if (buscarGalaxiaPorId(id) != NULL) {
        cout << "Error: el ID de galaxia ya esta registrado." << endl;
        return false;
    }
    if (buscarGalaxiaPorCodigo(codigo) != NULL) {
        cout << "Error: el codigo de galaxia ya esta registrado." << endl;
        return false;
    }
    if (buscarGalaxiaPorNombre(nombre) != NULL) {
        cout << "Error: el nombre de galaxia ya esta registrado." << endl;
        return false;
    }

    Galaxia* nueva = new Galaxia;
    nueva->id = id;
    nueva->codigo = codigo;
    nueva->nombre = nombre;
    nueva->tipo = tipo;
    nueva->descripcion = descripcion;
    nueva->x = x;
    nueva->y = y;
    nueva->z = z;
    nueva->arcos = NULL;
    nueva->siguiente = NULL;

    if (primeraGalaxia == NULL) primeraGalaxia = nueva;
    else {
        Galaxia* actual = primeraGalaxia;
        while (actual->siguiente != NULL) actual = actual->siguiente;
        actual->siguiente = nueva;
    }
    cout << "Galaxia insertada correctamente." << endl;
    return true;
}

void mostrarGalaxias() {
    if (primeraGalaxia == NULL) {
        cout << "No hay galaxias registradas." << endl;
        return;
    }
    Galaxia* actual = primeraGalaxia;
    while (actual != NULL) {
        cout << actual->id << " | " << actual->codigo << " | "
             << actual->nombre << " | " << actual->tipo << " | ("
             << actual->x << ", " << actual->y << ", " << actual->z << ")" << endl;
        actual = actual->siguiente;
    }
}

void liberarGalaxias() {
    while (primeraGalaxia != NULL) {
        Galaxia* borrar = primeraGalaxia;
        primeraGalaxia = primeraGalaxia->siguiente;
        delete borrar;
    }
}

Ruta* buscarRuta(string id) {
    Ruta* actual = primeraRuta;
    while (actual != NULL) {
        if (normalizar(actual->id) == normalizar(id)) return actual;
        actual = actual->siguiente;
    }
    return NULL;
}

bool existeConexion(Galaxia* origen, Galaxia* destino, bool dirigida, Ruta* ignorar = NULL) {
    Ruta* actual = primeraRuta;
    while (actual != NULL) {
        if (actual != ignorar) {
            bool directa = actual->origen == origen && actual->destino == destino;
            bool inversa = actual->origen == destino && actual->destino == origen;
            if (directa) return true;
            if (inversa && (!dirigida || !actual->dirigida)) return true;
        }
        actual = actual->siguiente;
    }
    return false;
}

void insertarArco(Galaxia* origen, Galaxia* destino, Ruta* ruta) {
    Arco* nuevo = new Arco;
    nuevo->destino = destino;
    nuevo->ruta = ruta;
    nuevo->siguiente = NULL;
    if (origen->arcos == NULL) origen->arcos = nuevo;
    else {
        Arco* actual = origen->arcos;
        while (actual->siguiente != NULL) actual = actual->siguiente;
        actual->siguiente = nuevo;
    }
}

bool insertarRuta(string id, string idOrigen, string idDestino,
                  double costo, bool dirigida) {
    Galaxia* origen = buscarGalaxia(idOrigen);
    Galaxia* destino = buscarGalaxia(idDestino);
    if (textoInvalido(id)) {
        cout << "Error: ID de ruta invalido." << endl;
        return false;
    }
    if (buscarRuta(id) != NULL) {
        cout << "Error: el ID de ruta ya esta registrado." << endl;
        return false;
    }
    if (origen == NULL || destino == NULL || origen == destino) {
        cout << "Error: origen o destino invalido." << endl;
        return false;
    }
    if (costo <= 0) {
        cout << "Error: el costo debe ser mayor que cero." << endl;
        return false;
    }
    if (existeConexion(origen, destino, dirigida)) {
        cout << "Error: ya existe una conexion equivalente." << endl;
        return false;
    }

    Ruta* nueva = new Ruta;
    nueva->id = id;
    nueva->origen = origen;
    nueva->destino = destino;
    nueva->costo = costo;
    nueva->dirigida = dirigida;
    nueva->siguiente = NULL;
    if (primeraRuta == NULL) primeraRuta = nueva;
    else {
        Ruta* actual = primeraRuta;
        while (actual->siguiente != NULL) actual = actual->siguiente;
        actual->siguiente = nueva;
    }
    insertarArco(origen, destino, nueva);
    if (!dirigida) insertarArco(destino, origen, nueva);
    cout << "Ruta insertada correctamente." << endl;
    return true;
}

void mostrarRutas() {
    Ruta* actual = primeraRuta;
    while (actual != NULL) {
        cout << actual->id << " | " << actual->origen->nombre << " -> "
             << actual->destino->nombre << " | costo: " << actual->costo
             << " | " << (actual->dirigida ? "dirigida" : "no dirigida") << endl;
        actual = actual->siguiente;
    }
}

void mostrarRutasDesde(string valor) {
    Galaxia* galaxia = buscarGalaxia(valor);
    if (galaxia == NULL) {
        cout << "Galaxia no encontrada." << endl;
        return;
    }
    Arco* arco = galaxia->arcos;
    while (arco != NULL) {
        cout << galaxia->nombre << " -> " << arco->destino->nombre
             << " | " << arco->ruta->id << " | " << arco->ruta->costo << endl;
        arco = arco->siguiente;
    }
}

void liberarRutasYArcos() {
    Galaxia* galaxia = primeraGalaxia;
    while (galaxia != NULL) {
        while (galaxia->arcos != NULL) {
            Arco* borrar = galaxia->arcos;
            galaxia->arcos = galaxia->arcos->siguiente;
            delete borrar;
        }
        galaxia = galaxia->siguiente;
    }
    while (primeraRuta != NULL) {
        Ruta* borrar = primeraRuta;
        primeraRuta = primeraRuta->siguiente;
        delete borrar;
    }
}

Nave* buscarNavePorId(string id) {
    Nave* actual = primeraNave;
    while (actual != NULL) {
        if (normalizar(actual->id) == normalizar(id)) return actual;
        actual = actual->siguiente;
    }
    return NULL;
}
Nave* buscarNavePorCodigo(string codigo) {
    Nave* actual = primeraNave;
    while (actual != NULL) {
        if (normalizar(actual->codigo) == normalizar(codigo)) return actual;
        actual = actual->siguiente;
    }
    return NULL;
}
Nave* buscarNavePorNombre(string nombre) {
    Nave* actual = primeraNave;
    while (actual != NULL) {
        if (normalizar(actual->nombre) == normalizar(nombre)) return actual;
        actual = actual->siguiente;
    }
    return NULL;
}
Nave* buscarNave(string valor) {
    Nave* nave = buscarNavePorId(valor);
    if (nave != NULL) return nave;
    nave = buscarNavePorCodigo(valor);
    if (nave != NULL) return nave;
    return buscarNavePorNombre(valor);
}
Viaje* buscarViaje(string id) {
    Viaje* actual = primerViaje;
    while (actual != NULL) {
        if (normalizar(actual->id) == normalizar(id)) return actual;
        actual = actual->siguiente;
    }
    return NULL;
}

bool insertarNave(string id, string codigo, string nombre) {
    if (textoInvalido(id) || textoInvalido(codigo) || textoInvalido(nombre)) {
        cout << "Error: datos de nave invalidos." << endl;
        return false;
    }
    if (buscarNavePorId(id) != NULL) { cout << "Error: ID de nave repetido." << endl; return false; }
    if (buscarNavePorCodigo(codigo) != NULL) { cout << "Error: codigo de nave repetido." << endl; return false; }
    if (buscarNavePorNombre(nombre) != NULL) { cout << "Error: nombre de nave repetido." << endl; return false; }
    Nave* nueva = new Nave{id, codigo, nombre, NULL};
    if (primeraNave == NULL) primeraNave = nueva;
    else {
        Nave* actual = primeraNave;
        while (actual->siguiente != NULL) actual = actual->siguiente;
        actual->siguiente = nueva;
    }
    cout << "Nave insertada correctamente." << endl;
    return true;
}

Viaje* insertarViaje(string id, string naveId, string origenId,
                     string destinoId, double costo, string fecha) {
    if (buscarViaje(id) != NULL) {
        cout << "Error: ID de viaje repetido." << endl;
        return NULL;
    }
    Nave* nave = buscarNave(naveId);
    Galaxia* origen = buscarGalaxia(origenId);
    Galaxia* destino = buscarGalaxia(destinoId);
    if (nave == NULL || origen == NULL || destino == NULL || origen == destino || costo <= 0) {
        cout << "Error: datos del viaje invalidos." << endl;
        return NULL;
    }
    Viaje* nuevo = new Viaje;
    nuevo->id = id;
    nuevo->fecha = fecha;
    nuevo->nave = nave;
    nuevo->origen = origen;
    nuevo->destino = destino;
    nuevo->costoTotal = costo;
    nuevo->rutasUsadas = NULL;
    nuevo->siguiente = NULL;
    if (primerViaje == NULL) primerViaje = nuevo;
    else {
        Viaje* actual = primerViaje;
        while (actual->siguiente != NULL) actual = actual->siguiente;
        actual->siguiente = nuevo;
    }
    return nuevo;
}

void agregarPasoViaje(Viaje* viaje, Ruta* ruta) {
    if (viaje == NULL || ruta == NULL) return;
    PasoViaje* nuevo = new PasoViaje{ruta, ruta->id, NULL};
    if (viaje->rutasUsadas == NULL) viaje->rutasUsadas = nuevo;
    else {
        PasoViaje* actual = viaje->rutasUsadas;
        while (actual->siguiente != NULL) actual = actual->siguiente;
        actual->siguiente = nuevo;
    }
}

void mostrarNaves() {
    Nave* actual = primeraNave;
    while (actual != NULL) {
        cout << actual->id << " | " << actual->codigo << " | " << actual->nombre << endl;
        actual = actual->siguiente;
    }
}

void mostrarHistorial() {
    Viaje* actual = primerViaje;
    while (actual != NULL) {
        cout << actual->id << " | " << actual->nave->nombre << " | "
             << actual->origen->nombre << " -> " << actual->destino->nombre
             << " | " << actual->costoTotal << " | " << actual->fecha << endl;
        actual = actual->siguiente;
    }
}

void liberarNavesYViajes() {
    while (primerViaje != NULL) {
        Viaje* borrar = primerViaje;
        primerViaje = primerViaje->siguiente;
        while (borrar->rutasUsadas != NULL) {
            PasoViaje* paso = borrar->rutasUsadas;
            borrar->rutasUsadas = borrar->rutasUsadas->siguiente;
            delete paso;
        }
        delete borrar;
    }
    while (primeraNave != NULL) {
        Nave* borrar = primeraNave;
        primeraNave = primeraNave->siguiente;
        delete borrar;
    }
}

bool rutaUsadaEnHistorial(Ruta* ruta) {
    Viaje* viaje = primerViaje;
    while (viaje != NULL) {
        PasoViaje* paso = viaje->rutasUsadas;
        while (paso != NULL) {
            if (paso->ruta == ruta) return true;
            paso = paso->siguiente;
        }
        viaje = viaje->siguiente;
    }
    return false;
}

bool modificarGalaxia(string valor, string nuevoCodigo, string nuevoNombre,
                      string nuevoTipo, string nuevaDescripcion,
                      double x, double y, double z) {
    Galaxia* galaxia = buscarGalaxia(valor);
    if (galaxia == NULL) return false;
    Galaxia* codigo = buscarGalaxiaPorCodigo(nuevoCodigo);
    Galaxia* nombre = buscarGalaxiaPorNombre(nuevoNombre);
    if (codigo != NULL && codigo != galaxia) {
        cout << "Error: codigo de galaxia repetido." << endl;
        return false;
    }
    if (nombre != NULL && nombre != galaxia) {
        cout << "Error: nombre de galaxia repetido." << endl;
        return false;
    }
    galaxia->codigo = nuevoCodigo;
    galaxia->nombre = nuevoNombre;
    galaxia->tipo = nuevoTipo;
    galaxia->descripcion = nuevaDescripcion;
    galaxia->x = x; galaxia->y = y; galaxia->z = z;
    return true;
}

bool modificarNave(string valor, string nuevoCodigo, string nuevoNombre) {
    Nave* nave = buscarNave(valor);
    if (nave == NULL) return false;
    Nave* codigo = buscarNavePorCodigo(nuevoCodigo);
    Nave* nombre = buscarNavePorNombre(nuevoNombre);
    if (codigo != NULL && codigo != nave) {
        cout << "Error: codigo de nave repetido." << endl;
        return false;
    }
    if (nombre != NULL && nombre != nave) {
        cout << "Error: nombre de nave repetido." << endl;
        return false;
    }
    nave->codigo = nuevoCodigo;
    nave->nombre = nuevoNombre;
    return true;
}

bool modificarRuta(string id, string origenId, string destinoId,
                   double costo, bool dirigida) {
    Ruta* ruta = buscarRuta(id);
    Galaxia* origen = buscarGalaxia(origenId);
    Galaxia* destino = buscarGalaxia(destinoId);
    if (ruta == NULL || origen == NULL || destino == NULL || origen == destino || costo <= 0) return false;
    if (rutaUsadaEnHistorial(ruta)) {
        cout << "Error: la ruta aparece en el historial." << endl;
        return false;
    }
    if (existeConexion(origen, destino, dirigida, ruta)) {
        cout << "Error: conexion repetida." << endl;
        return false;
    }
    ruta->origen = origen;
    ruta->destino = destino;
    ruta->costo = costo;
    ruta->dirigida = dirigida;
    return true;
}

bool eliminarViaje(string id) {
    Viaje* actual = primerViaje;
    Viaje* anterior = NULL;
    while (actual != NULL && normalizar(actual->id) != normalizar(id)) {
        anterior = actual;
        actual = actual->siguiente;
    }
    if (actual == NULL) return false;
    if (anterior == NULL) primerViaje = actual->siguiente;
    else anterior->siguiente = actual->siguiente;
    while (actual->rutasUsadas != NULL) {
        PasoViaje* paso = actual->rutasUsadas;
        actual->rutasUsadas = actual->rutasUsadas->siguiente;
        delete paso;
    }
    delete actual;
    return true;
}

vector<Galaxia*> obtenerVectorGalaxias() {
    vector<Galaxia*> lista;
    Galaxia* actual = primeraGalaxia;
    while (actual != NULL) {
        lista.push_back(actual);
        actual = actual->siguiente;
    }
    return lista;
}

int indiceGalaxia(const vector<Galaxia*>& lista, Galaxia* buscada) {
    for (int i = 0; i < (int)lista.size(); i++) if (lista[i] == buscada) return i;
    return -1;
}

ResultadoCamino dijkstra(string origenId, string destinoId) {
    ResultadoCamino resultado;
    resultado.encontrado = false;
    resultado.costoTotal = 0;
    Galaxia* origen = buscarGalaxia(origenId);
    Galaxia* destino = buscarGalaxia(destinoId);
    if (origen == NULL || destino == NULL) return resultado;

    vector<Galaxia*> vertices = obtenerVectorGalaxias();
    int n = (int)vertices.size();
    vector<double> distancia(n, INFINITO);
    vector<bool> visitado(n, false);
    vector<int> anterior(n, -1);
    vector<Ruta*> rutaAnterior(n, NULL);
    int inicio = indiceGalaxia(vertices, origen);
    int final = indiceGalaxia(vertices, destino);
    distancia[inicio] = 0;

    for (int paso = 0; paso < n; paso++) {
        int menor = -1;
        for (int i = 0; i < n; i++) {
            if (!visitado[i] && (menor == -1 || distancia[i] < distancia[menor])) menor = i;
        }
        if (menor == -1 || distancia[menor] == INFINITO) break;
        visitado[menor] = true;
        Arco* arco = vertices[menor]->arcos;
        while (arco != NULL) {
            int vecino = indiceGalaxia(vertices, arco->destino);
            double nueva = distancia[menor] + arco->ruta->costo;
            if (vecino != -1 && !visitado[vecino] && nueva < distancia[vecino]) {
                distancia[vecino] = nueva;
                anterior[vecino] = menor;
                rutaAnterior[vecino] = arco->ruta;
            }
            arco = arco->siguiente;
        }
    }
    if (distancia[final] == INFINITO) return resultado;

    vector<Galaxia*> invertidas;
    vector<Ruta*> rutasInvertidas;
    int actual = final;
    while (actual != -1) {
        invertidas.push_back(vertices[actual]);
        if (rutaAnterior[actual] != NULL) rutasInvertidas.push_back(rutaAnterior[actual]);
        actual = anterior[actual];
    }
    for (int i = (int)invertidas.size() - 1; i >= 0; i--) resultado.galaxias.push_back(invertidas[i]);
    for (int i = (int)rutasInvertidas.size() - 1; i >= 0; i--) resultado.rutas.push_back(rutasInvertidas[i]);
    resultado.encontrado = true;
    resultado.costoTotal = distancia[final];
    cantidadRutasCalculadas++;
    return resultado;
}

void mostrarResultadoDijkstra(ResultadoCamino resultado) {
    if (!resultado.encontrado) {
        cout << "No existe una ruta entre las galaxias." << endl;
        return;
    }
    for (int i = 0; i < (int)resultado.galaxias.size(); i++) {
        cout << resultado.galaxias[i]->nombre;
        if (i + 1 < (int)resultado.galaxias.size()) cout << " -> ";
    }
    cout << " | costo total: " << resultado.costoTotal << endl;
}

int buscarPadre(vector<int>& padre, int posicion) {
    while (padre[posicion] != posicion) posicion = padre[posicion];
    return posicion;
}
void unirConjuntos(vector<int>& padre, int a, int b) {
    int raizA = buscarPadre(padre, a);
    int raizB = buscarPadre(padre, b);
    padre[raizB] = raizA;
}
vector<Ruta*> crearOrdenAleatorioLocal() {
    vector<Ruta*> rutas;
    Ruta* actual = primeraRuta;
    while (actual != NULL) {
        rutas.push_back(actual);
        actual = actual->siguiente;
    }
    for (int i = (int)rutas.size() - 1; i > 0; i--) {
        int posicion = rand() % (i + 1);
        Ruta* temporal = rutas[i];
        rutas[i] = rutas[posicion];
        rutas[posicion] = temporal;
    }
    return rutas;
}
vector<Ruta*> kruskalModificado() {
    vector<Galaxia*> galaxias = obtenerVectorGalaxias();
    vector<Ruta*> aristas = ordenKruskal.empty() ? crearOrdenAleatorioLocal() : ordenKruskal;
    vector<Ruta*> arbol;
    vector<int> padre(galaxias.size());
    for (int i = 0; i < (int)padre.size(); i++) padre[i] = i;
    for (int i = 0; i < (int)aristas.size(); i++) {
        int origen = indiceGalaxia(galaxias, aristas[i]->origen);
        int destino = indiceGalaxia(galaxias, aristas[i]->destino);
        if (origen == -1 || destino == -1) continue;
        int conjuntoOrigen = buscarPadre(padre, origen);
        int conjuntoDestino = buscarPadre(padre, destino);
        if (conjuntoOrigen != conjuntoDestino) {
            arbol.push_back(aristas[i]);
            unirConjuntos(padre, conjuntoOrigen, conjuntoDestino);
        }
        if ((int)arbol.size() == (int)galaxias.size() - 1) break;
    }
    return arbol;
}
void mostrarKruskal() {
    vector<Ruta*> arbol = kruskalModificado();
    double total = 0;
    for (int i = 0; i < (int)arbol.size(); i++) {
        cout << arbol[i]->origen->nombre << " -- " << arbol[i]->destino->nombre
             << " | " << arbol[i]->costo << endl;
        total += arbol[i]->costo;
    }
    cout << "Costo total: " << total << endl;
}

void guardarGalaxias() {
    ofstream archivo("galaxias.txt");
    Galaxia* actual = primeraGalaxia;
    while (actual != NULL) {
        archivo << actual->id << "|" << actual->codigo << "|" << actual->nombre << "|"
                << actual->x << "|" << actual->y << "|" << actual->z << "|"
                << actual->tipo << "|" << actual->descripcion << endl;
        actual = actual->siguiente;
    }
}
void guardarRutas() {
    ofstream archivo("rutas.txt");
    Ruta* actual = primeraRuta;
    while (actual != NULL) {
        archivo << actual->id << "|" << actual->origen->id << "|" << actual->destino->id
                << "|" << actual->costo << "|" << actual->dirigida << endl;
        actual = actual->siguiente;
    }
}
void guardarNaves() {
    ofstream archivo("naves.txt");
    Nave* actual = primeraNave;
    while (actual != NULL) {
        archivo << actual->id << "|" << actual->codigo << "|" << actual->nombre << endl;
        actual = actual->siguiente;
    }
}
void guardarHistorial() {
    ofstream archivo("historial.txt");
    Viaje* actual = primerViaje;
    while (actual != NULL) {
        archivo << actual->id << "|" << actual->nave->id << "|" << actual->origen->id
                << "|" << actual->destino->id << "|" << actual->costoTotal << "|"
                << actual->fecha << "|";
        PasoViaje* paso = actual->rutasUsadas;
        while (paso != NULL) {
            archivo << paso->idRuta;
            if (paso->siguiente != NULL) archivo << ",";
            paso = paso->siguiente;
        }
        archivo << endl;
        actual = actual->siguiente;
    }
}
void guardarDatosLocales() {
    guardarGalaxias(); guardarRutas(); guardarNaves(); guardarHistorial();
}

void cargarGalaxiasArchivo() {
    ifstream archivo("galaxias.txt");
    string linea;
    while (getline(archivo, linea)) {
        stringstream ss(linea);
        string id, codigo, nombre, x, y, z, tipo, descripcion;
        getline(ss, id, '|'); getline(ss, codigo, '|'); getline(ss, nombre, '|');
        getline(ss, x, '|'); getline(ss, y, '|'); getline(ss, z, '|');
        getline(ss, tipo, '|'); getline(ss, descripcion, '|');
        insertarGalaxia(id, codigo, nombre, tipo, descripcion,
                        atof(x.c_str()), atof(y.c_str()), atof(z.c_str()));
    }
}
void cargarRutasArchivo() {
    ifstream archivo("rutas.txt");
    string linea;
    while (getline(archivo, linea)) {
        stringstream ss(linea);
        string id, origen, destino, costo, dirigida;
        getline(ss, id, '|'); getline(ss, origen, '|'); getline(ss, destino, '|');
        getline(ss, costo, '|'); getline(ss, dirigida, '|');
        insertarRuta(id, origen, destino, atof(costo.c_str()), atoi(dirigida.c_str()) == 1);
    }
}
void cargarNavesArchivo() {
    ifstream archivo("naves.txt");
    string linea;
    while (getline(archivo, linea)) {
        stringstream ss(linea);
        string id, codigo, nombre;
        getline(ss, id, '|'); getline(ss, codigo, '|'); getline(ss, nombre, '|');
        insertarNave(id, codigo, nombre);
    }
}
void cargarDatosLocales() {
    cargarGalaxiasArchivo();
    cargarRutasArchivo();
    cargarNavesArchivo();
}

size_t guardarRespuesta(void* contenido, size_t tamano, size_t cantidad, void* destino) {
    size_t total = tamano * cantidad;
    ((string*)destino)->append((char*)contenido, total);
    return total;
}

string consumirEndpoint(string endpoint) {
    CURL* curl = curl_easy_init();
    if (curl == NULL) return "";
    string respuesta = "";
    string url = URL_BASE + endpoint;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, guardarRespuesta);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &respuesta);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 90L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ProyectoGrafos/1.0");
    CURLcode resultado = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    if (resultado != CURLE_OK) return "";
    return respuesta;
}

json obtenerJson(string endpoint) {
    string texto = consumirEndpoint(endpoint);
    if (texto == "") return json();
    return json::parse(texto, NULL, false);
}

json obtenerArreglo(json datos, string llave) {
    if (datos.is_array()) return datos;
    if (datos.is_object() && datos.contains(llave) && datos[llave].is_array()) return datos[llave];
    return json::array();
}

int main() {
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        cout << "No se pudo inicializar libcurl." << endl;
        return 1;
    }

    json datos = obtenerJson("/grafo");
    if (!datos.is_null() && !datos.empty()) {
        cout << "Conexion con la API realizada correctamente." << endl;
    } else {
        cout << "No fue posible obtener datos de la API." << endl;
    }

    curl_global_cleanup();
    return 0;
}

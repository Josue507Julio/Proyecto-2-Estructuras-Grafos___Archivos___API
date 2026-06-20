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

void liberarPasos(PasoViaje* paso) {
    while (paso != NULL) {
        PasoViaje* borrar = paso;
        paso = paso->siguiente;
        delete borrar;
    }
}

void agregarPasoHistorico(Viaje* viaje, string idRuta) {
    if (viaje == NULL || idRuta == "") return;

    PasoViaje* nuevo = new PasoViaje;
    nuevo->ruta = buscarRuta(idRuta);
    nuevo->idRuta = idRuta;
    nuevo->siguiente = NULL;

    if (viaje->rutasUsadas == NULL) viaje->rutasUsadas = nuevo;
    else {
        PasoViaje* actual = viaje->rutasUsadas;
        while (actual->siguiente != NULL) actual = actual->siguiente;
        actual->siguiente = nuevo;
    }
}

void mostrarViajeDetalle(Viaje* viaje) {
    if (viaje == NULL) return;

    cout << "ID: " << viaje->id
         << " | Nave: " << viaje->nave->nombre
         << " | " << viaje->origen->nombre << " -> " << viaje->destino->nombre
         << " | Costo: " << viaje->costoTotal
         << " | Fecha: " << viaje->fecha << endl;

    cout << "Rutas utilizadas: ";
    PasoViaje* paso = viaje->rutasUsadas;
    if (paso == NULL) cout << "No especificadas";

    while (paso != NULL) {
        cout << paso->idRuta;
        if (paso->ruta == NULL) cout << " (historica o inactiva)";
        if (paso->siguiente != NULL) cout << " -> ";
        paso = paso->siguiente;
    }
    cout << endl;
}

void mostrarHistorialNave(string valorNave) {
    Nave* nave = buscarNave(valorNave);
    if (nave == NULL) {
        cout << "La nave no existe." << endl;
        return;
    }

    bool encontrado = false;
    Viaje* actual = primerViaje;
    while (actual != NULL) {
        if (actual->nave == nave) {
            mostrarViajeDetalle(actual);
            encontrado = true;
        }
        actual = actual->siguiente;
    }

    if (!encontrado) cout << "La nave no posee viajes registrados." << endl;
}

void reconstruirGrafo() {
    Galaxia* galaxia = primeraGalaxia;
    while (galaxia != NULL) {
        while (galaxia->arcos != NULL) {
            Arco* borrar = galaxia->arcos;
            galaxia->arcos = galaxia->arcos->siguiente;
            delete borrar;
        }
        galaxia = galaxia->siguiente;
    }

    Ruta* ruta = primeraRuta;
    while (ruta != NULL) {
        insertarArco(ruta->origen, ruta->destino, ruta);
        if (!ruta->dirigida) insertarArco(ruta->destino, ruta->origen, ruta);
        ruta = ruta->siguiente;
    }
}

bool galaxiaTieneRutas(Galaxia* galaxia) {
    Ruta* ruta = primeraRuta;
    while (ruta != NULL) {
        if (ruta->origen == galaxia || ruta->destino == galaxia) return true;
        ruta = ruta->siguiente;
    }
    return false;
}

bool galaxiaUsadaEnHistorial(Galaxia* galaxia) {
    Viaje* viaje = primerViaje;
    while (viaje != NULL) {
        if (viaje->origen == galaxia || viaje->destino == galaxia) return true;
        viaje = viaje->siguiente;
    }
    return false;
}

bool naveTieneViajes(Nave* nave) {
    Viaje* viaje = primerViaje;
    while (viaje != NULL) {
        if (viaje->nave == nave) return true;
        viaje = viaje->siguiente;
    }
    return false;
}

bool eliminarGalaxia(string valor) {
    Galaxia* objetivo = buscarGalaxia(valor);
    if (objetivo == NULL) {
        cout << "La galaxia no existe." << endl;
        return false;
    }
    if (galaxiaTieneRutas(objetivo)) {
        cout << "No se puede eliminar: tiene rutas asociadas." << endl;
        return false;
    }
    if (galaxiaUsadaEnHistorial(objetivo)) {
        cout << "No se puede eliminar: aparece en el historial." << endl;
        return false;
    }

    Galaxia* actual = primeraGalaxia;
    Galaxia* anterior = NULL;
    while (actual != NULL && actual != objetivo) {
        anterior = actual;
        actual = actual->siguiente;
    }

    if (anterior == NULL) primeraGalaxia = actual->siguiente;
    else anterior->siguiente = actual->siguiente;
    delete actual;
    return true;
}

bool eliminarRuta(string id) {
    Ruta* actual = primeraRuta;
    Ruta* anterior = NULL;

    while (actual != NULL && normalizar(actual->id) != normalizar(id)) {
        anterior = actual;
        actual = actual->siguiente;
    }

    if (actual == NULL) {
        cout << "La ruta no existe." << endl;
        return false;
    }
    if (rutaUsadaEnHistorial(actual)) {
        cout << "No se puede eliminar: aparece en el historial." << endl;
        return false;
    }

    if (anterior == NULL) primeraRuta = actual->siguiente;
    else anterior->siguiente = actual->siguiente;

    delete actual;
    reconstruirGrafo();
    ordenKruskal.clear();
    return true;
}

bool eliminarNave(string valor) {
    Nave* objetivo = buscarNave(valor);
    if (objetivo == NULL) {
        cout << "La nave no existe." << endl;
        return false;
    }
    if (naveTieneViajes(objetivo)) {
        cout << "No se puede eliminar: posee viajes registrados." << endl;
        return false;
    }

    Nave* actual = primeraNave;
    Nave* anterior = NULL;
    while (actual != NULL && actual != objetivo) {
        anterior = actual;
        actual = actual->siguiente;
    }

    if (anterior == NULL) primeraNave = actual->siguiente;
    else anterior->siguiente = actual->siguiente;
    delete actual;
    return true;
}

bool registrarViajePorRutaMinima(string id, string naveId,
                                 string origenId, string destinoId,
                                 string fecha) {
    if (buscarViaje(id) != NULL) {
        cout << "Error: el ID de viaje ya esta registrado." << endl;
        return false;
    }

    Nave* nave = buscarNave(naveId);
    Galaxia* origen = buscarGalaxia(origenId);
    Galaxia* destino = buscarGalaxia(destinoId);

    if (nave == NULL || origen == NULL || destino == NULL) {
        cout << "La nave o alguna galaxia no existe." << endl;
        return false;
    }

    ResultadoCamino resultado = dijkstra(origen->id, destino->id);
    if (!resultado.encontrado) {
        cout << "No existe un camino entre las galaxias." << endl;
        return false;
    }

    Viaje* nuevo = insertarViaje(id, nave->id, origen->id, destino->id,
                                 resultado.costoTotal, fecha);
    if (nuevo == NULL) return false;

    for (int i = 0; i < (int)resultado.rutas.size(); i++) {
        agregarPasoViaje(nuevo, resultado.rutas[i]);
    }
    return true;
}

bool modificarViajeCompleto(string id, string naveId,
                            string origenId, string destinoId,
                            string fecha) {
    Viaje* viaje = buscarViaje(id);
    Nave* nave = buscarNave(naveId);
    Galaxia* origen = buscarGalaxia(origenId);
    Galaxia* destino = buscarGalaxia(destinoId);

    if (viaje == NULL || nave == NULL || origen == NULL || destino == NULL) {
        cout << "No fue posible asociar los datos del viaje." << endl;
        return false;
    }

    ResultadoCamino resultado = dijkstra(origen->id, destino->id);
    if (!resultado.encontrado) return false;

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
    return true;
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

void cargarGalaxiasJson(json lista) {
    for (int i = 0; i < (int)lista.size(); i++) {
        json dato = lista[i];
        insertarGalaxia(dato.value("id", ""), dato.value("codigo", ""),
                        dato.value("nombre", ""), dato.value("tipo", "No especificado"),
                        dato.value("descripcion", ""), dato.value("x", 0.0),
                        dato.value("y", 0.0), dato.value("z", 0.0));
    }
}

void cargarRutasJson(json lista, bool dirigida) {
    for (int i = 0; i < (int)lista.size(); i++) {
        json dato = lista[i];
        insertarRuta(dato.value("id", ""), dato.value("origen_id", ""),
                     dato.value("destino_id", ""), dato.value("costo", 0.0), dirigida);
    }
}

void cargarNavesJson(json lista) {
    for (int i = 0; i < (int)lista.size(); i++) {
        json dato = lista[i];
        insertarNave(dato.value("id", ""), dato.value("codigo", dato.value("id", "")),
                     dato.value("nombre", "Nave sin nombre"));
    }
}

bool cargarDatosApi() {
    json grafo = obtenerJson("/grafo");
    if (grafo.is_null() || grafo.is_discarded()) return false;
    json galaxias = obtenerArreglo(grafo, "nodos");
    json rutas = obtenerArreglo(grafo, "aristas");
    bool dirigida = false;
    if (grafo.contains("meta") && grafo["meta"].contains("es_dirigido"))
        dirigida = grafo["meta"]["es_dirigido"].get<bool>();
    cargarGalaxiasJson(galaxias);
    cargarRutasJson(rutas, dirigida);
    json naves = obtenerJson("/naves");
    cargarNavesJson(obtenerArreglo(naves, "naves"));
    guardarDatosLocales();
    return primeraGalaxia != NULL && primeraRuta != NULL;
}

bool existenArchivosLocalesCompletos() {
    ifstream galaxias("galaxias.txt");
    ifstream rutas("rutas.txt");
    ifstream naves("naves.txt");
    ifstream historial("historial.txt");
    return galaxias.good() && rutas.good() && naves.good() && historial.good();
}

void cargarHistorialArchivo() {
    ifstream archivo("historial.txt");
    string linea;

    while (getline(archivo, linea)) {
        stringstream ss(linea);
        string id, nave, origen, destino, costo, fecha, rutas;

        getline(ss, id, '|');
        getline(ss, nave, '|');
        getline(ss, origen, '|');
        getline(ss, destino, '|');
        getline(ss, costo, '|');
        getline(ss, fecha, '|');
        getline(ss, rutas, '|');

        Viaje* viaje = insertarViaje(id, nave, origen, destino,
                                     atof(costo.c_str()), fecha);
        if (viaje == NULL) continue;

        stringstream lista(rutas);
        string idRuta;
        while (getline(lista, idRuta, ',')) {
            if (idRuta != "") agregarPasoHistorico(viaje, idRuta);
        }
    }
}

void cargarDatosLocalesCompletos() {
    if (!existenArchivosLocalesCompletos()) {
        cout << "No existen todos los archivos locales requeridos." << endl;
        return;
    }

    cargarGalaxiasArchivo();
    cargarRutasArchivo();
    cargarNavesArchivo();
    cargarHistorialArchivo();
    cout << "Datos cargados desde archivos locales." << endl;
}

string jsonATexto(json valor) {
    if (valor.is_string()) return valor.get<string>();
    if (valor.is_number_integer()) return to_string(valor.get<long long>());
    if (valor.is_number_float()) {
        stringstream ss;
        ss << valor.get<double>();
        return ss.str();
    }
    if (valor.is_object()) {
        if (valor.contains("id")) return jsonATexto(valor["id"]);
        if (valor.contains("codigo")) return jsonATexto(valor["codigo"]);
        if (valor.contains("nombre")) return jsonATexto(valor["nombre"]);
    }
    return "";
}

string campoTexto(json dato, const vector<string>& llaves) {
    for (int i = 0; i < (int)llaves.size(); i++) {
        if (dato.contains(llaves[i]) && !dato[llaves[i]].is_null()) {
            return jsonATexto(dato[llaves[i]]);
        }
    }
    return "";
}

double jsonANumero(json valor) {
    if (valor.is_number()) return valor.get<double>();
    if (valor.is_string()) return atof(valor.get<string>().c_str());
    return 0;
}

double campoNumero(json dato, const vector<string>& llaves) {
    for (int i = 0; i < (int)llaves.size(); i++) {
        if (dato.contains(llaves[i]) && !dato[llaves[i]].is_null()) {
            return jsonANumero(dato[llaves[i]]);
        }
    }
    return 0;
}

Ruta* buscarRutaEntre(string origenId, string destinoId) {
    Galaxia* origen = buscarGalaxia(origenId);
    Galaxia* destino = buscarGalaxia(destinoId);
    if (origen == NULL || destino == NULL) return NULL;

    Ruta* actual = primeraRuta;
    while (actual != NULL) {
        bool directa = actual->origen == origen && actual->destino == destino;
        bool inversa = !actual->dirigida && actual->origen == destino && actual->destino == origen;
        if (directa || inversa) return actual;
        actual = actual->siguiente;
    }
    return NULL;
}

void cargarHistorialJsonCompleto(json lista) {
    int insertados = 0;
    int omitidos = 0;

    for (int i = 0; i < (int)lista.size(); i++) {
        json dato = lista[i];
        string id = campoTexto(dato, {"id", "viaje_id", "viajeId"});
        string naveId = campoTexto(dato, {"nave_id", "naveId", "nave"});
        string origenRef = campoTexto(dato, {"origen_id", "origenId", "origen_nombre", "origen"});
        string destinoRef = campoTexto(dato, {"destino_id", "destinoId", "destino_nombre", "destino"});
        string fecha = campoTexto(dato, {"fecha", "date", "fecha_viaje"});
        string rutaId = campoTexto(dato, {"ruta_id", "rutaId", "ruta"});
        double costo = campoNumero(dato, {"costo_real", "costo", "tiempo", "duracion"});

        Ruta* ruta = buscarRuta(rutaId);
        if (ruta != NULL) {
            if (origenRef == "") origenRef = ruta->origen->id;
            if (destinoRef == "") destinoRef = ruta->destino->id;
            if (costo <= 0) costo = ruta->costo;
        }

        Galaxia* origen = buscarGalaxia(origenRef);
        Galaxia* destino = buscarGalaxia(destinoRef);
        Nave* nave = buscarNave(naveId);

        if (id == "" || nave == NULL || origen == NULL || destino == NULL || costo <= 0) {
            omitidos++;
            continue;
        }

        Viaje* viaje = insertarViaje(id, nave->id, origen->id, destino->id, costo, fecha);
        if (viaje == NULL) {
            omitidos++;
            continue;
        }

        if (rutaId != "") agregarPasoHistorico(viaje, rutaId);
        insertados++;
    }

    cout << "Viajes de API insertados: " << insertados
         << " | Omitidos: " << omitidos << endl;
}

void cargarOrdenKruskalJsonCompleto(json lista) {
    ordenKruskal.clear();

    for (int i = 0; i < (int)lista.size(); i++) {
        json dato = lista[i];
        string idRuta;

        if (dato.is_string()) idRuta = dato.get<string>();
        else idRuta = campoTexto(dato, {"id", "ruta_id", "rutaId"});

        Ruta* ruta = buscarRuta(idRuta);
        if (ruta == NULL && dato.is_object()) {
            string origen = campoTexto(dato, {"origen_id", "origenId", "origen"});
            string destino = campoTexto(dato, {"destino_id", "destinoId", "destino"});
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

bool cargarDatosApiCompleta() {
    cout << "Cargando informacion desde la API..." << endl;

    json grafo = obtenerJson("/grafo");
    if (grafo.is_null() || grafo.is_discarded() || grafo.empty()) return false;

    json galaxias = obtenerArreglo(grafo, "nodos");
    json rutas = obtenerArreglo(grafo, "aristas");
    bool dirigida = false;

    if (grafo.contains("meta") && grafo["meta"].is_object() &&
        grafo["meta"].contains("es_dirigido")) {
        dirigida = grafo["meta"]["es_dirigido"].get<bool>();
    }

    cargarGalaxiasJson(galaxias);
    cargarRutasJson(rutas, dirigida);

    json respuestaNaves = obtenerJson("/naves");
    json naves = obtenerArreglo(respuestaNaves, "naves");
    if (naves.size() == 0) naves = obtenerArreglo(respuestaNaves, "data");
    cargarNavesJson(naves);

    json respuestaHistorial = obtenerJson("/historial");
    json historial = obtenerArreglo(respuestaHistorial, "historial");
    if (historial.size() == 0) historial = obtenerArreglo(respuestaHistorial, "viajes");
    if (historial.size() == 0) historial = obtenerArreglo(respuestaHistorial, "data");
    cargarHistorialJsonCompleto(historial);

    json respuestaKruskal = obtenerJson("/grafo/kruskal");
    json kruskal = obtenerArreglo(respuestaKruskal, "aristas");
    if (kruskal.size() == 0) kruskal = obtenerArreglo(respuestaKruskal, "rutas");
    if (kruskal.size() == 0) kruskal = respuestaKruskal;
    if (kruskal.is_array()) cargarOrdenKruskalJsonCompleto(kruskal);

    guardarDatosLocales();
    return primeraGalaxia != NULL && primeraRuta != NULL;
}

int main() {
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) return 1;
    cout << "Cargando datos desde la API..." << endl;
    if (!cargarDatosApi()) {
        cout << "No fue posible cargar la API. Se usan archivos locales." << endl;
        cargarDatosLocales();
    }
    mostrarGalaxias();
    mostrarRutas();
    liberarNavesYViajes();
    liberarRutasYArcos();
    liberarGalaxias();
    curl_global_cleanup();
    return 0;
}

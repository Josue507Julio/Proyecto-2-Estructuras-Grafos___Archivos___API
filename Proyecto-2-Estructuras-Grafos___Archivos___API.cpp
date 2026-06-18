/*
    PROYECTO PROGRAMADO: GRAFOS Y ARCHIVOS
    Sistema de rutas intergalacticas

    Fecha de inicio: 17/06/2026
    Ultima modificacion: 17/06/2026
    Integrantes: 
        Yohan Andrey Morera Ramírez     2025105204
        Jaiden Joel Avila Badilla       2025119253
        Josué David González Alvarado   2024297026

    Descripcion:
    El programa consume una API con libcurl y procesa la respuesta JSON.
    Las galaxias, rutas, naves y viajes se almacenan con estructuras struct,
    listas enlazadas y punteros. El grafo se representa mediante una lista
    de galaxias y una sublista de arcos para cada galaxia.
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

using namespace std;

const double INFINITO = 999999999.0;

struct Ruta;
struct Galaxia;

struct Arco {
    Galaxia* destino;
    Ruta* ruta;
    Arco* siguiente;
};

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

Galaxia* primeraGalaxia = NULL;
Ruta* primeraRuta = NULL;

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

int main() {
    insertarGalaxia("galaxia-1", "GAL-001", "Via Lactea", "espiral", "", 0, 0, 0);
    insertarGalaxia("galaxia-2", "GAL-002", "Andromeda", "espiral", "", 10, 5, 2);
    insertarGalaxia("galaxia-3", "GAL-003", "Triangulo", "espiral", "", 4, 9, 1);
    insertarRuta("ruta-1", "galaxia-1", "galaxia-2", 12, false);
    insertarRuta("ruta-2", "galaxia-2", "galaxia-3", 8, true);
    mostrarRutas();
    mostrarRutasDesde("galaxia-2");
    liberarRutasYArcos();
    liberarGalaxias();
    return 0;
}

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
    string id;
    string codigo;
    string nombre;
    string tipo;
    string descripcion;
    double x;
    double y;
    double z;
    Arco* arcos;
    Galaxia* siguiente;
};

Galaxia* primeraGalaxia = NULL;

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

void liberarGalaxias() {
    while (primeraGalaxia != NULL) {
        Galaxia* borrar = primeraGalaxia;
        primeraGalaxia = primeraGalaxia->siguiente;
        delete borrar;
    }
}

int main() {
    insertarGalaxia("galaxia-1", "GAL-001", "Via Lactea", "espiral",
                    "Galaxia de prueba", 0, 0, 0);
    Galaxia* encontrada = buscarGalaxia("GAL-001");
    if (encontrada != NULL) {
        cout << "Galaxia encontrada: " << encontrada->nombre << endl;
    }
    liberarGalaxias();
    return 0;
}

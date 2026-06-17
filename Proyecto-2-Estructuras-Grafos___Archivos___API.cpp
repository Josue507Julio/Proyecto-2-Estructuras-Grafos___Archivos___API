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

struct Galaxia {
    string id;
    string codigo;
    string nombre;
    string tipo;
    string descripcion;
    double x;
    double y;
    double z;
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

int main() {
    cout << "Sistema de rutas galacticas iniciado." << endl;
    cout << "Estructura Galaxia y funciones de entrada configuradas." << endl;
    return 0;
}

#ifndef RECOMENDADOR_H
#define RECOMENDADOR_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <algorithm>
#include <queue>
#include <string>
#include <iomanip>

struct Valoracion {
    int user_id;
    int song_id;
    float rating;
};

struct EstadoRatings {
    std::unordered_map<int, std::unordered_map<int, float>> ratings_usuario;
    std::unordered_map<int, std::unordered_map<int, float>> ratings_cancion;
    std::unordered_map<int, std::unordered_set<int>> canciones_por_usuario;
};

// Funciones principales
std::vector<Valoracion> leerCSV(const std::string& archivo);
EstadoRatings construirEstructuras(const std::vector<Valoracion>& valoraciones);
void agregarValoracion(EstadoRatings& estado, const Valoracion& v);
void eliminarValoracion(EstadoRatings& estado, int user_id, int song_id);
void modificarValoracion(EstadoRatings& estado, int user_id, int song_id, float nuevo_rating);

// Top P usuarios más similares a un usuario dado
std::vector<std::pair<int, double>> usuariosSimilares(const EstadoRatings& estado, int user_id, int topP);

// Similitud directa entre dos usuarios
double similitudEntreUsuarios(
    int u1, int u2,
    const std::unordered_map<int, std::unordered_map<int, float>>& ratings_por_usuario
);
/// Devuelve las canciones mejor valoradas globalmente (por promedio bayesiano)
std::vector<std::pair<int, double>> topCancionesGlobales(const EstadoRatings& estado, int topN);

/// Devuelve las canciones mejor valoradas por promedio simple (sin corrección bayesiana)
std::vector<std::pair<int, double>> topCancionesPorPromedio(const EstadoRatings& estado, int topN);

float calcularSimilitud(
    int c1, int c2,
    const std::unordered_map<int, std::unordered_map<int, float>>& ratings_por_cancion,
    int& usuarios_comunes
);

std::vector<std::pair<int, float>> recomendarEscalable(
    int usuario,
    const EstadoRatings& estado,
    int topN,
    int topSimilaresPorFavorita,
    float umbralRatingFavorita,
    int minUsuariosCandidata,
    bool usar_ajuste,
    std::unordered_map<int, std::vector<std::string>>& explicacion
);

void imprimirExplicaciones(
    int usuario,
    const std::vector<std::pair<int, float>>& recomendaciones,
    const std::unordered_map<int, std::vector<std::string>>& explicacion,
    bool usar_ajuste
);

#endif // RECOMENDADOR_H

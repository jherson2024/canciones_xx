#include "recomendador.h"
std::vector<Valoracion> leerCSV(const std::string& archivo) {
    std::ifstream f(archivo);
    std::vector<Valoracion> datos;
    std::string linea;
    while (std::getline(f, linea)) {
        std::stringstream ss(linea);
        std::string token;
        Valoracion v;
        std::getline(ss, token, ',');
        v.user_id = std::stoi(token);
        std::getline(ss, token, ',');
        v.song_id = std::stoi(token);
        std::getline(ss, token, ',');
        v.rating = std::stof(token);
        datos.push_back(v);
    }
    return datos;
}
std::vector<std::pair<int, double>> usuariosSimilares(const EstadoRatings& estado, int user_id, int topP) {
    std::vector<std::pair<int, double>> similares;
    const auto& ratings_usuario = estado.ratings_usuario;

    for (const auto& [otro_id, _] : ratings_usuario) {
        if (otro_id == user_id) continue;
        double sim = similitudEntreUsuarios(user_id, otro_id, ratings_usuario);
        similares.emplace_back(otro_id, sim);
    }

    std::sort(similares.begin(), similares.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    if (similares.size() > topP) similares.resize(topP);
    return similares;
}
double similitudEntreUsuarios(
    int u1, int u2,
    const std::unordered_map<int, std::unordered_map<int, float>>& ratings_por_usuario
) {
    const auto& r1 = ratings_por_usuario.at(u1);
    const auto& r2 = ratings_por_usuario.at(u2);

    double num = 0, denom1 = 0, denom2 = 0;

    for (const auto& [song_id, rating1] : r1) {
        auto it = r2.find(song_id);
        if (it != r2.end()) {
            double rating2 = it->second;
            num += rating1 * rating2;
            denom1 += rating1 * rating1;
            denom2 += rating2 * rating2;
        }
    }

    if (num == 0 || denom1 == 0 || denom2 == 0) return 0;

    double sim = num / (std::sqrt(denom1) * std::sqrt(denom2));
    return std::min(sim, 1.0); // Clamp para evitar errores numéricos
}

std::vector<std::pair<int, double>> topCancionesGlobales(const EstadoRatings& estado, int topN) {
    std::vector<std::pair<int, double>> promedios;
    double suma_total = 0;
    int total_ratings = 0;

    // Calcular promedio global
    for (const auto& [_, ratings] : estado.ratings_cancion) {
        for (const auto& [__, rating] : ratings) {
            suma_total += rating;
            total_ratings++;
        }
    }
    double global_avg = total_ratings > 0 ? suma_total / total_ratings : 0;
    int m = 10; // número mínimo de votos deseado (puedes ajustarlo)

    // Calcular promedio bayesiano para cada canción
    for (const auto& [cancion_id, ratings] : estado.ratings_cancion) {
        int v = ratings.size();
        double suma = 0;
        for (const auto& [_, rating] : ratings)
            suma += rating;

        double promedio = suma / v;
        double bayes_avg = (v * promedio + m * global_avg) / (v + m);

        promedios.emplace_back(cancion_id, bayes_avg);
    }

    std::sort(promedios.begin(), promedios.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    if (promedios.size() > topN) promedios.resize(topN);
    return promedios;
}

std::vector<std::pair<int, double>> topCancionesPorPromedio(const EstadoRatings& estado, int N) {
    std::vector<std::pair<int, double>> promedios;

    for (const auto& [cancion_id, ratings] : estado.ratings_cancion) {
        double suma = 0;
        for (const auto& [_, rating] : ratings)
            suma += rating;

        double promedio = suma / ratings.size();
        promedios.emplace_back(cancion_id, promedio);
    }

    std::sort(promedios.begin(), promedios.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    if (promedios.size() > N) promedios.resize(N);
    return promedios;
}
EstadoRatings construirEstructuras(const std::vector<Valoracion>& valoraciones) {
    EstadoRatings estado;
    for (const auto& v : valoraciones) {
        estado.ratings_usuario[v.user_id][v.song_id] = v.rating;
        estado.ratings_cancion[v.song_id][v.user_id] = v.rating;
        estado.canciones_por_usuario[v.user_id].insert(v.song_id);
    }
    return estado;
}
void agregarValoracion(EstadoRatings& estado, const Valoracion& v) {
    estado.ratings_usuario[v.user_id][v.song_id] = v.rating;
    estado.ratings_cancion[v.song_id][v.user_id] = v.rating;
    estado.canciones_por_usuario[v.user_id].insert(v.song_id);
}
void eliminarValoracion(EstadoRatings& estado, int user_id, int song_id) {
    estado.ratings_usuario[user_id].erase(song_id);
    estado.ratings_cancion[song_id].erase(user_id);
    estado.canciones_por_usuario[user_id].erase(song_id);
}
void modificarValoracion(EstadoRatings& estado, int user_id, int song_id, float nuevo_rating) {
    estado.ratings_usuario[user_id][song_id] = nuevo_rating;
    estado.ratings_cancion[song_id][user_id] = nuevo_rating;
    estado.canciones_por_usuario[user_id].insert(song_id);
}


float calcularSimilitud(
    int c1, int c2,
    const std::unordered_map<int, std::unordered_map<int, float>>& ratings_por_cancion,
    int& usuarios_comunes
) {
    const auto& u1 = ratings_por_cancion.at(c1);
    const auto& u2 = ratings_por_cancion.at(c2);
    float num = 0, denom1 = 0, denom2 = 0;
    usuarios_comunes = 0;
    for (const auto& [user_id, r1] : u1) {
        auto it = u2.find(user_id);
        if (it != u2.end()) {
            float r2 = it->second;
            num += r1 * r2;
            denom1 += r1 * r1;
            denom2 += r2 * r2;
            usuarios_comunes++;
        }
    }
    if (usuarios_comunes == 0 || denom1 == 0 || denom2 == 0) return 0;
    return num / (std::sqrt(denom1) * std::sqrt(denom2));
}

std::vector<std::pair<int, float>> recomendarEscalable(
    int usuario,
    const EstadoRatings& estado,
    int topN,
    int topSimilaresPorFavorita,
    float umbralRatingFavorita,
    int minUsuariosCandidata,
    bool usar_ajuste,
    std::unordered_map<int, std::vector<std::string>>& explicacion
) {
    const auto& ratings_usuario = estado.ratings_usuario;
    const auto& ratings_cancion = estado.ratings_cancion;
    const auto& canciones_por_usuario = estado.canciones_por_usuario;

    std::unordered_set<int> canciones_usuario;
    for (int c : canciones_por_usuario.at(usuario))
        canciones_usuario.insert(c);

    float promedio_usuario = 0;
    if (usar_ajuste) {
        for (const auto& [_, r] : ratings_usuario.at(usuario))
            promedio_usuario += r;
        promedio_usuario /= ratings_usuario.at(usuario).size();
    }

    std::unordered_map<int, float> acumulador;

    for (const auto& [cancion_fav, rating_fav] : ratings_usuario.at(usuario)) {
        if (rating_fav < umbralRatingFavorita) continue;
        std::unordered_map<int, int> contador_candidatos;
        for (const auto& [user_id, _] : ratings_cancion.at(cancion_fav)) {
            for (int c : canciones_por_usuario.at(user_id)) {
                if (!canciones_usuario.count(c))
                    contador_candidatos[c]++;
            }
        }
        std::priority_queue<std::pair<int, int>> pq;
        for (const auto& [c, count] : contador_candidatos) {
            pq.emplace(count, c);
        }
        int contador = 0;
        while (!pq.empty() && contador < topSimilaresPorFavorita) {
            int candidata = pq.top().second;
            pq.pop();
            contador++;
            if (ratings_cancion.at(candidata).size() < minUsuariosCandidata) continue;
            int comunes = 0;
            float sim = calcularSimilitud(cancion_fav, candidata, ratings_cancion, comunes);
            if (sim > 0) {
                float score = usar_ajuste
                    ? promedio_usuario + sim * (rating_fav - promedio_usuario)
                    : sim * rating_fav;
                acumulador[candidata] += score;
                std::ostringstream oss;
                oss << "↳ desde favorita " << cancion_fav
                    << " (sim=" << std::fixed << std::setprecision(3) << sim
                    << ", rating=" << rating_fav;
                if (usar_ajuste) {
                    oss << ", ajuste=" << score;
                } else {
                    oss << ", score=" << score;
                }
                oss << ", comunes=" << comunes << ")";
                explicacion[candidata].push_back(oss.str());
            }
        }
    }
    std::vector<std::pair<int, float>> recomendaciones(acumulador.begin(), acumulador.end());
    std::sort(recomendaciones.begin(), recomendaciones.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    if (recomendaciones.size() > topN) recomendaciones.resize(topN);
    return recomendaciones;
}   
void imprimirExplicaciones(
    int usuario,
    const std::vector<std::pair<int, float>>& recomendaciones,
    const std::unordered_map<int, std::vector<std::string>>& explicacion,
    bool usar_ajuste
) {
    std::cout << "\n🔍 Recomendaciones para el usuario " << usuario;
    std::cout << (usar_ajuste ? " (con ajuste personalizado)" : " (score simple)") << ":\n";
    if (recomendaciones.empty()) {
        std::cout << "⚠️  No se encontraron recomendaciones.\n";
        return;
    }
    for (const auto& [cancion, score] : recomendaciones) {
        std::cout << "🎵 Canción " << cancion << " (score: " << std::fixed << std::setprecision(2) << score << ")\n";
        for (const auto& detalle : explicacion.at(cancion)) {
            std::cout << "    " << detalle << "\n";
        }
    }
}

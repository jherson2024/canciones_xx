#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include "recomendador.h"

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

EstadoRatings estado;

// ================== HANDLER GET ===================

void handle_get(http_request request) {
    auto path = uri::decode(request.relative_uri().path());
    auto query = uri::split_query(request.request_uri().query());

    json::value response;

    try {
        if (path == U("/top")) {
            int n = std::stoi(query[U("n")]);
            auto top = topCancionesGlobales(estado, n);
            json::value arr = json::value::array();
            for (size_t i = 0; i < top.size(); ++i) {
                arr[i][U("cancion")] = json::value::number(top[i].first);
                arr[i][U("promedio")] = json::value::number(top[i].second);
            }
            request.reply(status_codes::OK, arr);
            return;
        }

        if (path == U("/similares")) {
            int user = std::stoi(query[U("user")]);
            int p = std::stoi(query[U("p")]);
            auto sim = usuariosSimilares(estado, user, p);
            json::value arr = json::value::array();
            for (size_t i = 0; i < sim.size(); ++i) {
                arr[i][U("usuario")] = json::value::number(sim[i].first);
                arr[i][U("similitud")] = json::value::number(sim[i].second);
            }
            request.reply(status_codes::OK, arr);
            return;
        }

        if (path == U("/similitud")) {
            int u1 = std::stoi(query[U("u1")]);
            int u2 = std::stoi(query[U("u2")]);
            double sim = similitudEntreUsuarios(u1, u2, estado.ratings_usuario);
            response[U("usuario1")] = json::value::number(u1);
            response[U("usuario2")] = json::value::number(u2);
            response[U("similitud")] = json::value::number(sim);
            request.reply(status_codes::OK, response);
            return;
        }

       if (path == U("/recomendar")) {
            int user = std::stoi(query[U("user")]);
            std::unordered_map<int, std::vector<std::string>> explicacion;
            auto recomendaciones = recomendarEscalable(user, estado, 6, 6, 4.0, 2, true, explicacion);

            json::value arr = json::value::array();
            for (size_t i = 0; i < recomendaciones.size(); ++i) {
                int idCancion = recomendaciones[i].first;
                float score = recomendaciones[i].second;

                json::value item = json::value::object();
                item[U("cancion")] = json::value::number(idCancion);
                item[U("score")] = json::value::number(score);

                // Si hay explicación para esta canción
                if (explicacion.count(idCancion)) {
                    json::value explicaciones = json::value::array();
                    const auto& razones = explicacion[idCancion];
                    for (size_t j = 0; j < razones.size(); ++j) {
                            explicaciones[j] = json::value::string(utility::conversions::to_string_t(razones[j]));
                    }
                    item[U("explicacion")] = explicaciones;
                }

                arr[i] = item;
            }

            request.reply(status_codes::OK, arr);
            return;
        }

        if (path == U("/agregar")) {
            int user = std::stoi(query[U("user")]);
            int song = std::stoi(query[U("song")]);
            float rating = std::stof(query[U("rating")]);
            agregarValoracion(estado, {user, song, rating});
            response[U("mensaje")] = json::value::string(U("Valoración agregada."));
            request.reply(status_codes::OK, response);
            return;
        }

        if (path == U("/eliminar")) {
            int user = std::stoi(query[U("user")]);
            int song = std::stoi(query[U("song")]);
            eliminarValoracion(estado, user, song);
            response[U("mensaje")] = json::value::string(U("Valoración eliminada."));
            request.reply(status_codes::OK, response);
            return;
        }

        if (path == U("/modificar")) {
            int user = std::stoi(query[U("user")]);
            int song = std::stoi(query[U("song")]);
            float rating = std::stof(query[U("rating")]);
            modificarValoracion(estado, user, song, rating);
            response[U("mensaje")] = json::value::string(U("Valoración modificada."));
            request.reply(status_codes::OK, response);
            return;
        }

        // ========== SERVIR ARCHIVOS ESTÁTICOS DESDE /public ==========

        std::string archivo = path == U("/") ? "index.html" : utility::conversions::to_utf8string(path.substr(1));
        std::string ruta = "public/" + archivo;

        std::ifstream file(ruta, std::ios::binary);
        if (!file) {
            response[U("error")] = json::value::string(U("Ruta no reconocida"));
            request.reply(status_codes::NotFound, response);
            return;
        }

        std::ostringstream buffer;
        buffer << file.rdbuf();
        std::string contenido = buffer.str();

        std::string content_type = "application/octet-stream";
        if (ruta.ends_with(".html"))       content_type = "text/html";
        else if (ruta.ends_with(".css"))   content_type = "text/css";
        else if (ruta.ends_with(".js"))    content_type = "application/javascript";
        else if (ruta.ends_with(".json"))  content_type = "application/json";
        else if (ruta.ends_with(".svg"))   content_type = "image/svg+xml";
        else if (ruta.ends_with(".png"))   content_type = "image/png";
        else if (ruta.ends_with(".jpg") || ruta.ends_with(".jpeg")) content_type = "image/jpeg";

        http_response staticResponse(status_codes::OK);
        staticResponse.headers().add(U("Content-Type"), utility::conversions::to_string_t(content_type));
        staticResponse.set_body(contenido);
        request.reply(staticResponse);

    } catch (const std::exception& e) {
        response[U("error")] = json::value::string(U("Error de parámetros: ") + utility::conversions::to_string_t(e.what()));
        request.reply(status_codes::BadRequest, response);
    }
}


// ================== MAIN ===================

int main() {
    std::cout << "📥 Cargando datos...\n";
    auto valoraciones = leerCSV("datos/ratings_s.csv");
    estado = construirEstructuras(valoraciones);
    std::cout << "✅ Datos cargados y estructuras construidas.\n";

    http_listener listener(U("http://localhost:5000"));
    listener.support(methods::GET, handle_get);

    try {
        listener.open().wait();
        std::cout << "🚀 Servidor iniciado en http://localhost:5000\n";
        std::cout << "🌐 Endpoints disponibles: /top, /similares, /similitud, /recomendar, /agregar, /eliminar, /modificar\n";
        std::cout << "Presiona ENTER para cerrar el servidor...\n";
        std::string dummy;
        std::getline(std::cin, dummy);
        listener.close().wait();
    } catch (const std::exception& e) {
        std::cerr << "❌ Error al iniciar el servidor: " << e.what() << std::endl;
    }

    return 0;
}

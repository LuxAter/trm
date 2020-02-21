#include "scene.hpp"

#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

#include "camera.hpp"
#include "material.hpp"
#include "rand.hpp"
#include "sdf.hpp"
#include "settings.hpp"
#include "type.hpp"

#include <nlohmann/json.hpp>

Float getf(const nlohmann::json &json) {
  if (json.is_string()) {
    std::string str = json.get<std::string>();
    if (!str.compare(0, 5, "rand(")) {
      if (str == "rand()")
        return trm::frand();
      else if (str.find(',') == std::string::npos) {
        Float maxv = 1.0f;
        std::sscanf(str.c_str(), "rand(%f)", &maxv);
        return trm::frand(0.0f, maxv);
      } else {
        Float minv = 0.0f, maxv = 1.0f;
        std::sscanf(str.c_str(), "rand(%f, %f)", &minv, &maxv);
        return trm::frand(minv, maxv);
      }
    }
    return 0.0f;
  } else {
    return json.get<Float>();
  }
}

bool trm::load_json(const std::string &file, RenderSettings *settings,
                    Scene *scene) {
  std::ifstream config_file(file);
  if (!config_file.is_open()) {
    std::fprintf(stderr, "Failed to load JSON file \"%s\"\n", file.c_str());
    return false;
  }
  nlohmann::json json;
  config_file >> json;
  config_file.close();

  if (json.contains("maximumDistance")) {
    settings->maximum_distance = json.at("maximumDistance").get<Float>();
  }
  if (json.contains("epsilonDistance")) {
    settings->epsilon_distance = json.at("epsilonDistance").get<Float>();
  }
  if (json.contains("resolution") &&
      (settings->resolution.x == 0 || settings->resolution.y == 0)) {
    nlohmann::json::iterator it = json.find("resolution");
    if (it->is_array() && it->size() == 2) {
      settings->resolution =
          uvec2(it->at(0).get<unsigned int>(), it->at(1).get<unsigned int>());
    } else if (it->is_number()) {
      settings->resolution = uvec2(it->get<unsigned int>());
    } else {
      std::fprintf(stderr, "Failed to parse \"resolution\"\n");
      return false;
    }
  }
  if (json.contains("maximumDepth") && settings->maximum_depth == 0) {
    settings->maximum_depth = json.at("maximumDepth").get<std::size_t>();
  }
  if (json.contains("spp") && settings->spp == 0) {
    settings->spp = json.at("spp").get<std::size_t>();
  }
  if (json.contains("progressBar") && settings->no_bar == false) {
    settings->no_bar = !json.at("progressBar").get<bool>();
  }
  if (json.contains("output") && settings->output_fmt == "") {
    settings->output_fmt = json.at("output").get<std::string>();
  }

  if (json.contains("camera")) {
    nlohmann::json::iterator it = json.find("camera");
    if (it->contains("fov")) {
      scene->camera.fov = it->at("fov").get<Float>();
    }
    if (it->contains("pos")) {
      scene->camera.pos = vec3(it->at("pos").at(0).get<Float>(),
                               it->at("pos").at(1).get<Float>(),
                               it->at("pos").at(2).get<Float>());
    }
    if (it->contains("center")) {
      scene->camera.center = vec3(it->at("center").at(0).get<Float>(),
                                  it->at("center").at(1).get<Float>(),
                                  it->at("center").at(2).get<Float>());
    }
    if (it->contains("up")) {
      scene->camera.up =
          vec3(it->at("up").at(0).get<Float>(), it->at("up").at(1).get<Float>(),
               it->at("up").at(2).get<Float>());
    }
  }

  std::map<std::string, std::shared_ptr<trm::Material>> material_index;
  if (json.contains("materials")) {
    nlohmann::json::iterator materials = json.find("materials");
    for (nlohmann::json::iterator it = materials->begin();
         it != materials->end(); ++it) {
      trm::Material::Shading shading = trm::Material::DIFF;
      Float emission = 0.0f, ior = 0.0f;
      Vec3 color(1.0f);
      if (it->contains("shading")) {
        std::string key = it->at("shading").get<std::string>();
        if (key == "DIFFUSE")
          shading = trm::Material::DIFF;
        else if (key == "SPECULAR")
          shading = trm::Material::SPEC;
        else if (key == "REFRACTIVE")
          shading = trm::Material::REFR;
        else if (key == "EMISSIVE")
          shading = trm::Material::EMIS;
      }
      if (it->contains("emission")) {
        emission = getf(it->at("emission"));
      }
      if (it->contains("ior")) {
        ior = getf(it->at("ior"));
      }
      if (it->contains("color")) {
        color = Vec3(getf(it->at("color").at(0)), getf(it->at("color").at(1)),
                     getf(it->at("color").at(2)));
      }
      std::printf("EMIS: %f, IOR: %f\n", emission, ior);
      scene->materials.push_back(
          std::make_shared<trm::Material>(shading, color, emission, ior));
      material_index[it.key()] = scene->materials.back();
    }
  }

  std::map<std::shared_ptr<trm::Sdf>, std::array<std::string, 2>>
      linked_objects;
  std::map<std::string, std::shared_ptr<trm::Sdf>> object_index;
  if (json.contains("objects")) {
    nlohmann::json::iterator objects = json.find("objects");
    for (nlohmann::json::iterator it = objects->begin(); it != objects->end();
         ++it) {
      std::shared_ptr<trm::Material> material_ptr =
          it->contains("material")
              ? material_index[it->at("material").get<std::string>()]
              : nullptr;
      std::string type =
          it->contains("type") ? it->at("type").get<std::string>() : "";
      if (type == "sphere") {
        scene->objects.push_back(
            sdfSphere(getf(it->at("radius")), material_ptr));
      } else if (type == "box") {
        scene->objects.push_back(
            sdfBox(getf(it->at("dim").at(0)), getf(it->at("dim").at(1)),
                   getf(it->at("dim").at(2)), material_ptr));
      } else if (type == "cylinder") {
        scene->objects.push_back(sdfCylinder(
            getf(it->at("height")), getf(it->at("radius")), material_ptr));
      } else if (type == "torus") {
        scene->objects.push_back(sdfTorus(getf(it->at("radiusRevolve")),
                                          getf(it->at("radius")),
                                          material_ptr));
      } else if (type == "plane") {
        scene->objects.push_back(sdfPlane(
            getf(it->at("normal").at(0)), getf(it->at("normal").at(1)),
            getf(it->at("normal").at(2)),
            it->at("normal").size() >= 4 ? getf(it->at("normal").at(3)) : 0.0f,
            material_ptr));
      } else if (type == "elongate") {
        scene->objects.push_back(sdfElongate(nullptr,
                                             Vec3(getf(it->at("scale").at(0)),
                                                  getf(it->at("scale").at(1)),
                                                  it->at("scale").at(2)),
                                             material_ptr));
        linked_objects[scene->objects.back()] = {
            it->at("object").get<std::string>(), ""};
      } else if (type == "round") {
        scene->objects.push_back(
            sdfRound(nullptr, getf(it->at("radius")), material_ptr));
        linked_objects[scene->objects.back()] = {
            it->at("object").get<std::string>(), ""};
      } else if (type == "onion") {
        scene->objects.push_back(
            sdfOnion(nullptr, getf(it->at("thickness")), material_ptr));
        linked_objects[scene->objects.back()] = {
            it->at("object").get<std::string>(), ""};
      } else if (type == "union") {
        scene->objects.push_back(sdfUnion(nullptr, nullptr, material_ptr));
        linked_objects[scene->objects.back()] = {
            it->at("a").get<std::string>(), it->at("b").get<std::string>()};
      } else if (type == "subtraction") {
        scene->objects.push_back(
            sdfSubtraction(nullptr, nullptr, material_ptr));
        linked_objects[scene->objects.back()] = {
            it->at("a").get<std::string>(), it->at("b").get<std::string>()};
      } else if (type == "intersection") {
        scene->objects.push_back(
            sdfIntersection(nullptr, nullptr, material_ptr));
        linked_objects[scene->objects.back()] = {
            it->at("a").get<std::string>(), it->at("b").get<std::string>()};
      } else if (type == "smoothUnion") {
        scene->objects.push_back(sdfSmoothUnion(
            nullptr, nullptr, getf(it->at("radius")), material_ptr));
        linked_objects[scene->objects.back()] = {
            it->at("a").get<std::string>(), it->at("b").get<std::string>()};
      } else if (type == "smoothSubtraction") {
        scene->objects.push_back(sdfSmoothSubtraction(
            nullptr, nullptr, getf(it->at("radius")), material_ptr));
        linked_objects[scene->objects.back()] = {
            it->at("a").get<std::string>(), it->at("b").get<std::string>()};
      } else if (type == "smoothIntersection") {
        scene->objects.push_back(sdfSmoothIntersection(
            nullptr, nullptr, getf(it->at("radius")), material_ptr));
        linked_objects[scene->objects.back()] = {
            it->at("a").get<std::string>(), it->at("b").get<std::string>()};
      } else {
        std::fprintf(stderr, "Every object must have a specific type\n");
        return false;
      }
      if (it->contains("scale")) {
        if (it->at("scale").is_number()) {
          scene->objects.back()->scale(Vec3(getf(it->at("scale"))));
        } else {
          scene->objects.back()->scale(Vec3(getf(it->at("scale").at(0)),
                                            getf(it->at("scale").at(1)),
                                            getf(it->at("scale").at(2))));
        }
      }
      if (it->contains("rotation")) {
        if (it->at("rotation").is_array()) {
          scene->objects.back()
              ->rotate(getf(it->at("rotation").at(0)), Vec3(1.0f, 0.0f, 0.0f))
              ->rotate(getf(it->at("rotation").at(1)), Vec3(0.0f, 1.0f, 0.0f))
              ->rotate(getf(it->at("rotation").at(2)), Vec3(0.0f, 0.0f, 1.0f));
        } else {
          nlohmann::json rotate = it->at("rotation");
          if (rotate.contains("x")) {
            scene->objects.back()->rotate(getf(rotate.at("x")),
                                          Vec3(1.0f, 0.0f, 0.0f));
          }
          if (rotate.contains("y")) {
            scene->objects.back()->rotate(getf(rotate.at("y")),
                                          Vec3(0.0f, 1.0f, 0.0f));
          }
          if (rotate.contains("z")) {
            scene->objects.back()->rotate(getf(rotate.at("z")),
                                          Vec3(0.0f, 0.0f, 1.0f));
          }
          if (rotate.contains("angle") && rotate.contains("axis")) {
            scene->objects.back()->rotate(getf(rotate.at("angle")),
                                          Vec3(getf(rotate.at("axis").at(0)),
                                               getf(rotate.at("axis").at(1)),
                                               getf(rotate.at("axis").at(2))));
          }
        }
      }
      if (it->contains("position")) {
        scene->objects.back()->translate(Vec3(getf(it->at("position").at(0)),
                                              getf(it->at("position").at(1)),
                                              getf(it->at("position").at(2))));
      }
      object_index[it.key()] = scene->objects.back();
    }
  }

  for (auto &obj : linked_objects) {
    std::cout << "HI!" << obj.first << object_index[obj.second[0]] << "<<<\n";
    obj.first->a = object_index[obj.second[0]];
    if (obj.second[1] != "")
      obj.first->b = object_index[obj.second[1]];
  }

  return true;
}

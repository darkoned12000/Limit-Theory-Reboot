// Copyright (C) 2025  darkoned12000
// SPDX-License-Identifier: GPL-3.0-or-later
// Part of the ltheory-old-test modernization effort (Revamp Work).
// See NOTICE and LICENSE.GPL. Original engine (c) Josh Parnell, public domain.

#include "Module/SoundEngine.h"
#include "Game/Camera.h"
#include "Game/Object.h"
#include "LTE/Location.h"
#include "LTE/Array.h"
#include "LTE/AutoPtr.h"
#include "LTE/Math.h"
#include "LTE/ProgramLog.h"

#include <SFML/Audio.hpp>
#include <unordered_map>
#include <vector>
#include <string>
#include <memory>

namespace {
  const String kDefaultSoundPath = "sound/";
  const float kDistanceScale = 50.0f;
  const float kVelocityScale = 1.0f / 50.0f;

  struct SoundSFMLImpl : public SoundT {
    std::unique_ptr<sf::Sound> sound;
    sf::SoundBuffer const* buffer;
    bool deleted;
    float pan;
    float pitch;
    float volume;
    bool looped;
    bool playing;

    SoundSFMLImpl(std::unique_ptr<sf::Sound> s, sf::SoundBuffer const* buf) : 
      sound(std::move(s)), buffer(buf), deleted(false), pan(0), pitch(1), volume(1), looped(false), playing(false) {
      if (sound) {
        volume = sound->getVolume() / 100.0f;
        looped = sound->isLooping();
      }
    }

    void Delete() override {
      deleted = true;
      if (sound) sound->stop();
    }

    bool IsFinished() const override {
      if (!sound) return true;
      return sound->getStatus() == sf::SoundSource::Status::Stopped;
    }

    bool IsLooped() const override {
      return looped;
    }

    float GetDuration() const override {
      if (!sound || !buffer) return 0.0f;
      return buffer->getDuration().asMilliseconds();
    }

    float GetPan() const override {
      return pan;
    }

    float GetPitch() const override {
      return pitch;
    }

    float GetVolume() const override {
      return volume;
    }

    void SetCursor(float position) override {
      if (sound) sound->setPlayingOffset(sf::milliseconds(position));
    }

    void SetPan(float pan) override {
      pan = Clamp(pan, -1.0f, 1.0f);
      this->pan = pan;
      // SFML 2.x doesn't expose pan directly (only 3D positioning).
      // We'll leave it as a no-op until SFML 3.x if we really need 2D panning, 
      // or we can simulate it with 3D position at fixed depth.
    }
    
    void SetPitch(float pitch) override {
      this->pitch = pitch;
      if (sound) sound->setPitch(pitch);
    }
    
    void SetPlaying(bool playing) override {
      this->playing = playing;
      if (!sound) return;
      if (playing) sound->play();
      else sound->pause();
    }
    
    void SetVolume(float volume) override {
      volume = Saturate(volume);
      this->volume = volume;
      if (sound) sound->setVolume(volume * 100.0f);
    }
  };

  struct Sound3DInstance {
    Reference<SoundSFMLImpl> sound;
    Object carrier;
    V3 offset;

    void Update(V3D const& camPos) {
      if (!sound || !sound->sound) return;

      V3D position = carrier
        ? (V3D)carrier->GetTransform().TransformPoint(offset)
        : (V3D)offset;

      V3 relative = (V3)(position - camPos);
      
      // SFML expects coordinates. We pass the relative position.
      sound->sound->setPosition({relative.x, relative.y, relative.z});
    }
  };

  struct SoundEngineSFMLImpl : public SoundEngine {
    std::unordered_map<std::string, std::unique_ptr<sf::SoundBuffer>> buffers;
    std::vector<Reference<SoundSFMLImpl>> sounds2D;
    std::vector<std::unique_ptr<Sound3DInstance>> sounds3D;
    V3D camPos;

    SoundEngineSFMLImpl() : camPos(0) {
      sf::Listener::setGlobalVolume(100.0f);
    }

    ~SoundEngineSFMLImpl() override {
      sounds2D.clear();
      sounds3D.clear();
      buffers.clear();
    }

    sf::SoundBuffer* GetBuffer(String const& name) {
      String path = kDefaultSoundPath + name;
      auto it = buffers.find(path.c_str());
      if (it != buffers.end()) {
        return it->second.get();
      }
      
      AutoPtr<Array<uchar>> arr = Location_Resource(path)->Read();
      if (!arr) {
        Log_Critical("Failed to read sound " + name);
        return nullptr;
      }

      auto buffer = std::make_unique<sf::SoundBuffer>();
      if (!buffer->loadFromMemory(arr->data(), arr->size())) {
        Log_Critical("Failed to decode sound " + name);
        return nullptr;
      }
      
      sf::SoundBuffer* rawBuf = buffer.get();
      buffers[path.c_str()] = std::move(buffer);
      return rawBuf;
    }

    char const* GetName() const override {
      return "SoundEngine (SFML)";
    }

    Sound Play(Array<float> const& buffer) override {
      // In FMOD, this was used for raw floating point PCM data.
      // We can load raw PCM into sf::SoundBuffer, but it expects 16-bit integers.
      // For now, we will stub it, as it is rarely used (often for synth generation).
      return new SoundSFMLImpl(nullptr, nullptr);    }

    Sound Play2D(String const& filename, float volume, bool looped) override {
      sf::SoundBuffer* buf = GetBuffer(filename);
      if (!buf) return new SoundSFMLImpl(nullptr, nullptr);

      float effectiveVolume = volume * masterVolume;

      auto snd = std::make_unique<sf::Sound>(*buf);
      snd->setRelativeToListener(true);
      snd->setPosition({0, 0, 0});

      auto s = new SoundSFMLImpl(std::move(snd), buf);
      s->SetVolume(effectiveVolume);
      s->sound->setLooping(looped);
      s->looped = looped;
      s->SetPlaying(true);
      
      sounds2D.push_back(s);
      return s;
    }

    Sound Play3D(
      String const& s,
      Object const& o,
      V3 const& v,
      float f,
      float distanceDiv,
      bool looped) override
    {
      sf::SoundBuffer* buf = GetBuffer(s);
      if (!buf) return new SoundSFMLImpl(nullptr, nullptr);

      float effectiveVolume = f * masterVolume;

      auto snd = std::make_unique<sf::Sound>(*buf);
      snd->setRelativeToListener(true); // We manage relative position manually 
      snd->setMinDistance(kDistanceScale * distanceDiv);
      snd->setAttenuation(1.0f); // Default linear attenuation in SFML

      auto sndImpl = new SoundSFMLImpl(std::move(snd), buf);
      sndImpl->SetVolume(effectiveVolume);
      sndImpl->sound->setLooping(looped);
      sndImpl->looped = looped;
      
      auto info = std::make_unique<Sound3DInstance>();
      info->carrier = o;
      info->offset = v;
      info->sound = sndImpl;
      info->Update(camPos);
      
      sndImpl->SetPlaying(true);
      
      Reference<SoundSFMLImpl> ret = sndImpl;
      sounds3D.push_back(std::move(info));
      
      return ret;
    }

    void Update() override {
      // Update Listener
      Camera const& camera = Camera_Get();
      if (camera) {
        camPos = camera->GetPos();
        // We set SFML listener to origin, because we manually subtract camPos from all 3D sounds
        // This avoids large coordinate floating point precision issues.
        sf::Listener::setPosition({0, 0, 0});
        
        V3 camLook = camera->GetLook();
        V3 camUp = camera->GetUp();
        sf::Listener::setDirection({camLook.x, camLook.y, camLook.z});
        sf::Listener::setUpVector({camUp.x, camUp.y, camUp.z});
      }

      // Cleanup 2D sounds
      for (auto it = sounds2D.begin(); it != sounds2D.end(); ) {
        if ((*it)->IsFinished() || (*it)->deleted) {
          (*it)->deleted = false;
          it = sounds2D.erase(it);
        } else {
          ++it;
        }
      }

      // Update and cleanup 3D sounds
      for (auto it = sounds3D.begin(); it != sounds3D.end(); ) {
        if (((*it)->carrier && (*it)->carrier->IsDeleted()) || 
            (*it)->sound->IsFinished() || 
            (*it)->sound->deleted) {
          (*it)->sound->deleted = false;
          it = sounds3D.erase(it);
        } else {
          (*it)->Update(camPos);
          ++it;
        }
      }
    }
  };
}

SoundEngine* SoundEngine_SFML() {
  return new SoundEngineSFMLImpl;
}

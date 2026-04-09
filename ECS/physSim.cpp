//#include "PhysSim.h"
//#include "Transform.h"
//#include <SDL3/SDL.h>
//#include <cmath>
//#include <algorithm>
//
//// ─────────────────────────────────────────────────────────────────────────────
////  Init
//// ─────────────────────────────────────────────────────────────────────────────
//void PhysSim::Init()
//{
//    // Ground plane – a wide static rect at the bottom of the screen
//    ECS::Entity ground = SpawnBody(SimShapeType::Rect,
//        (float)_data->GAME_WIDTH * 0.5f,
//        (float)_data->GAME_HEIGHT - 30.0f);
//
//    SimBody& gb = ecs.Get<SimBody>(ground);
//    gb.hw = (float)_data->GAME_WIDTH * 0.5f;
//    gb.hh = 28.0f;
//    gb.isStatic = true;
//    gb.mass = 0.0f;
//
//    TransformComponent& gt = ecs.Get<TransformComponent>(ground);
//    gt.position.x = (float)_data->GAME_WIDTH * 0.5f;
//    gt.position.y = (float)_data->GAME_HEIGHT - 30.0f;
//}
//
//// ─────────────────────────────────────────────────────────────────────────────
////  Update
//// ─────────────────────────────────────────────────────────────────────────────
//void PhysSim::Update(float dt)
//{
//    // ── raw mouse position ────────────────────────────────────────────────
//    _mouseX = _data->inputs.GetActionAxis("mousePosX")[0];
//    _mouseY = _data->inputs.GetActionAxis("mousePosY")[0];
//
//    // Fallback: read directly from SDL if the action isn't wired up
//    // (PhysSim sets up its own action map in Init so this should work,
//    //  but we guard here just in case)
//    {
//        float mx, my;
//        SDL_GetMouseState(&mx, &my);
//        _mouseX = mx;
//        _mouseY = my;
//    }
//
//    bool lmb = (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) != 0;
//    bool rmb = (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT)) != 0;
//
//    const bool* keys = SDL_GetKeyboardState(nullptr);
//    bool keyS = keys[SDL_SCANCODE_S] != 0;
//    bool keyR = keys[SDL_SCANCODE_R] != 0;
//    bool keyF = keys[SDL_SCANCODE_F] != 0;
//    bool keyShift = keys[SDL_SCANCODE_LSHIFT] != 0;
//    bool keyDel = keys[SDL_SCANCODE_DELETE] != 0;
//    bool keySpace = keys[SDL_SCANCODE_SPACE] != 0;
//
//    // edge detection helpers
//    auto pressed = [](bool cur, bool& prev) -> bool {
//        bool edge = cur && !prev;
//        prev = cur;
//        return edge;
//        };
//
//    bool hitS = pressed(keyS, _prevS);
//    bool hitR = pressed(keyR, _prevR);
//    bool hitF = pressed(keyF, _prevF);
//    bool hitShift = pressed(keyShift, _prevShift);
//    bool hitDel = pressed(keyDel, _prevDel);
//    bool hitSpace = pressed(keySpace, _prevSpace);
//    bool lmbDown = lmb && !_prevLMB;
//    bool lmbUp = !lmb && _prevLMB;
//    _prevLMB = lmb;
//
//    // ── toggle force visibility ───────────────────────────────────────────
//    if (hitShift)
//        _showForces = !_showForces;
//
//    // ── space: start / reset simulation ──────────────────────────────────
//    if (hitSpace)
//    {
//        if (!_running)
//        {
//            SnapshotInitial();   // save positions before first run
//            _running = true;
//        }
//        else
//        {
//            _running = false;
//            ResetToInitial();
//        }
//        _mode = PhysSimMode::Idle;
//        _selectedBody = 0;
//        _selectedForce = -1;
//        _forceAnchored = false;
//    }
//
//    // ── mode switching (only when not running) ────────────────────────────
//    if (!_running)
//    {
//        if (hitS)
//        {
//            _mode = (_mode == PhysSimMode::PlacingCircle)
//                ? PhysSimMode::Idle : PhysSimMode::PlacingCircle;
//            _selectedBody = 0;
//            _selectedForce = -1;
//        }
//        if (hitR)
//        {
//            _mode = (_mode == PhysSimMode::PlacingRect)
//                ? PhysSimMode::Idle : PhysSimMode::PlacingRect;
//            _selectedBody = 0;
//            _selectedForce = -1;
//        }
//        if (hitF)
//        {
//            _mode = (_mode == PhysSimMode::PlacingForce)
//                ? PhysSimMode::Idle : PhysSimMode::PlacingForce;
//            _selectedBody = 0;
//            _selectedForce = -1;
//            _forceAnchored = false;
//        }
//
//        // ── placement clicks ──────────────────────────────────────────────
//        if (_mode == PhysSimMode::PlacingCircle && lmbDown)
//        {
//            SpawnBody(SimShapeType::Circle, _mouseX, _mouseY);
//            _mode = PhysSimMode::Idle;
//        }
//        else if (_mode == PhysSimMode::PlacingRect && lmbDown)
//        {
//            SpawnBody(SimShapeType::Rect, _mouseX, _mouseY);
//            _mode = PhysSimMode::Idle;
//        }
//        else if (_mode == PhysSimMode::PlacingForce)
//        {
//            if (!_forceAnchored && lmbDown)
//            {
//                // first click: must land on a body
//                ECS::Entity hit = HitTestBody(_mouseX, _mouseY);
//                if (hit != 0)
//                {
//                    _forceTarget = hit;
//                    _forceAnchorX = _mouseX;
//                    _forceAnchorY = _mouseY;
//                    _forceAnchored = true;
//                }
//            }
//            else if (_forceAnchored && lmbUp)
//            {
//                // release: commit force
//                float dx = _mouseX - _forceAnchorX;
//                float dy = _mouseY - _forceAnchorY;
//                // only add if the drag was meaningful
//                if (std::abs(dx) > 2.0f || std::abs(dy) > 2.0f)
//                {
//                    SimForce sf;
//                    sf.target = _forceTarget;
//                    sf.fx = dx / FORCE_SCALE;
//                    sf.fy = dy / FORCE_SCALE;
//                    sf.anchorX = _forceAnchorX;
//                    sf.anchorY = _forceAnchorY;
//                    _forces.push_back(sf);
//                }
//                _forceAnchored = false;
//                _mode = PhysSimMode::Idle;
//            }
//        }
//        else if (_mode == PhysSimMode::Idle && lmbDown)
//        {
//            // ── selection ─────────────────────────────────────────────────
//            _selectedBody = 0;
//            _selectedForce = -1;
//
//            // deselect all
//            for (auto e : _bodies)
//                ecs.Get<SimBody>(e).selected = false;
//            for (auto& f : _forces)
//                f.selected = false;
//
//            // try force first (arrows are thin, check them first)
//            if (_showForces)
//            {
//                int fi = HitTestForce(_mouseX, _mouseY);
//                if (fi >= 0)
//                {
//                    _selectedForce = fi;
//                    _forces[fi].selected = true;
//                }
//            }
//
//            if (_selectedForce < 0)
//            {
//                ECS::Entity hit = HitTestBody(_mouseX, _mouseY);
//                if (hit != 0)
//                {
//                    _selectedBody = hit;
//                    ecs.Get<SimBody>(hit).selected = true;
//                }
//            }
//        }
//
//        // ── delete ────────────────────────────────────────────────────────
//        if (hitDel)
//        {
//            if (_selectedBody != 0)
//                DeleteSelectedBody();
//            else if (_selectedForce >= 0)
//                DeleteSelectedForce();
//        }
//    }
//
//    // ── physics step ─────────────────────────────────────────────────────
//    if (_running)
//        StepPhysics(dt);
//}
//
//// ─────────────────────────────────────────────────────────────────────────────
////  Render
//// ─────────────────────────────────────────────────────────────────────────────
//void PhysSim::Render(float dt)
//{
//    SDL_Renderer* r = _data->SDLrenderer;
//
//    // background
//    SDL_SetRenderDrawColor(r, 18, 18, 28, 255);
//    SDL_RenderClear(r);
//
//    // ── bodies ────────────────────────────────────────────────────────────
//    for (auto e : _bodies)
//    {
//        if (!ecs.Alive(e)) continue;
//        const SimBody& b = ecs.Get<SimBody>(e);
//        const TransformComponent& t = ecs.Get<TransformComponent>(e);
//        DrawBody(b, t.position.x, t.position.y);
//    }
//
//    // ── committed force arrows ────────────────────────────────────────────
//    if (_showForces)
//    {
//        for (std::size_t i = 0; i < _forces.size(); i++)
//        {
//            const SimForce& sf = _forces[i];
//            if (!ecs.Alive(sf.target)) continue;
//            float ex = sf.anchorX + sf.fx * FORCE_SCALE;
//            float ey = sf.anchorY + sf.fy * FORCE_SCALE;
//            DrawForceArrow(sf.anchorX, sf.anchorY, ex, ey,
//                sf.selected, false);
//        }
//    }
//
//    // ── live force preview while dragging ────────────────────────────────
//    if (_mode == PhysSimMode::PlacingForce && _forceAnchored)
//    {
//        DrawForceArrow(_forceAnchorX, _forceAnchorY,
//            _mouseX, _mouseY,
//            false, true);
//    }
//
//    // ── ghost preview of object about to be placed ───────────────────────
//    if (!_running)
//    {
//        if (_mode == PhysSimMode::PlacingCircle)
//        {
//            SDL_SetRenderDrawColor(r, 100, 200, 255, 120);
//            DrawCircle(_mouseX, _mouseY, 30.0f);
//        }
//        else if (_mode == PhysSimMode::PlacingRect)
//        {
//            SDL_FRect ghost = { _mouseX - 30.0f, _mouseY - 30.0f, 60.0f, 60.0f };
//            SDL_SetRenderDrawColor(r, 100, 255, 160, 120);
//            SDL_RenderFillRect(r, &ghost);
//        }
//    }
//
//    // ── HUD ───────────────────────────────────────────────────────────────
//    // (Text rendering requires a font; we keep it minimal with coloured rects
//    //  as mode indicators in the top-left corner.)
//    auto drawIndicator = [&](int slot, Uint8 R, Uint8 G, Uint8 B, bool active)
//        {
//            SDL_FRect box = { 10.0f + slot * 28.0f, 10.0f, 22.0f, 22.0f };
//            SDL_SetRenderDrawColor(r, R, G, B, active ? 255 : 60);
//            SDL_RenderFillRect(r, &box);
//            SDL_SetRenderDrawColor(r, 255, 255, 255, 80);
//            SDL_RenderRect(r, &box);
//        };
//
//    drawIndicator(0, 100, 200, 255, _mode == PhysSimMode::PlacingCircle);   // S – circle
//    drawIndicator(1, 100, 255, 160, _mode == PhysSimMode::PlacingRect);     // R – rect
//    drawIndicator(2, 255, 220, 80, _mode == PhysSimMode::PlacingForce);    // F – force
//    drawIndicator(3, 80, 255, 80, _running);                              // SPACE – running
//    drawIndicator(4, 255, 255, 255, _showForces);                           // Shift – forces
//
//    SDL_RenderPresent(r);
//}
//
//// ─────────────────────────────────────────────────────────────────────────────
////  Physics
//// ─────────────────────────────────────────────────────────────────────────────
//void PhysSim::StepPhysics(float dt)
//{
//    // Apply forces + gravity → acceleration → velocity → position
//    for (auto e : _bodies)
//    {
//        if (!ecs.Alive(e)) continue;
//        SimBody& b = ecs.Get<SimBody>(e);
//        if (b.isStatic) continue;
//
//        b.ax = 0.0f;
//        b.ay = GRAVITY;
//
//        // accumulated applied forces
//        for (const SimForce& sf : _forces)
//        {
//            if (sf.target != e) continue;
//            if (b.mass > 0.0f)
//            {
//                b.ax += sf.fx / b.mass;
//                b.ay += sf.fy / b.mass;
//            }
//        }
//
//        b.vx += b.ax * dt;
//        b.vy += b.ay * dt;
//
//        TransformComponent& t = ecs.Get<TransformComponent>(e);
//        t.position.x += b.vx * dt;
//        t.position.y += b.vy * dt;
//    }
//
//    ResolveCollisions();
//}
//
//// ─── Collision helpers ────────────────────────────────────────────────────────
//
//struct ContactResult
//{
//    bool  hit = false;
//    float nx = 0.0f, ny = 0.0f;   // normal pointing from B into A
//    float depth = 0.0f;
//};
//
//static ContactResult CircleVsCircle(float ax, float ay, float ar,
//    float bx, float by, float br)
//{
//    float dx = ax - bx, dy = ay - by;
//    float distSq = dx * dx + dy * dy;
//    float radSum = ar + br;
//    if (distSq >= radSum * radSum) return {};
//    float dist = std::sqrt(distSq);
//    ContactResult c;
//    c.hit = true;
//    c.depth = radSum - dist;
//    if (dist < 0.0001f) { c.nx = 1.0f; c.ny = 0.0f; }
//    else { c.nx = dx / dist; c.ny = dy / dist; }
//    return c;
//}
//
//static ContactResult CircleVsAABB(float cx, float cy, float cr,
//    float rx, float ry, float rhw, float rhh)
//{
//    // closest point on AABB to circle centre
//    float closestX = std::max(rx - rhw, std::min(cx, rx + rhw));
//    float closestY = std::max(ry - rhh, std::min(cy, ry + rhh));
//    float dx = cx - closestX, dy = cy - closestY;
//    float distSq = dx * dx + dy * dy;
//    if (distSq >= cr * cr) return {};
//    float dist = std::sqrt(distSq);
//    ContactResult c;
//    c.hit = true;
//    c.depth = cr - dist;
//    if (dist < 0.0001f) { c.nx = 0.0f; c.ny = -1.0f; }
//    else { c.nx = dx / dist; c.ny = dy / dist; }
//    return c;
//}
//
//static ContactResult AABBvsAABB(float ax, float ay, float ahw, float ahh,
//    float bx, float by, float bhw, float bhh)
//{
//    float dx = ax - bx, dy = ay - by;
//    float overlapX = (ahw + bhw) - std::abs(dx);
//    float overlapY = (ahh + bhh) - std::abs(dy);
//    if (overlapX <= 0 || overlapY <= 0) return {};
//    ContactResult c;
//    c.hit = true;
//    if (overlapX < overlapY)
//    {
//        c.depth = overlapX;
//        c.nx = (dx > 0) ? 1.0f : -1.0f;
//        c.ny = 0.0f;
//    }
//    else
//    {
//        c.depth = overlapY;
//        c.nx = 0.0f;
//        c.ny = (dy > 0) ? 1.0f : -1.0f;
//    }
//    return c;
//}
//
//void PhysSim::ResolveCollisions()
//{
//    for (std::size_t i = 0; i < _bodies.size(); i++)
//    {
//        if (!ecs.Alive(_bodies[i])) continue;
//        SimBody& A = ecs.Get<SimBody>(_bodies[i]);
//        TransformComponent& tA = ecs.Get<TransformComponent>(_bodies[i]);
//
//        for (std::size_t j = i + 1; j < _bodies.size(); j++)
//        {
//            if (!ecs.Alive(_bodies[j])) continue;
//            SimBody& B = ecs.Get<SimBody>(_bodies[j]);
//            TransformComponent& tB = ecs.Get<TransformComponent>(_bodies[j]);
//
//            if (A.isStatic && B.isStatic) continue;
//
//            ContactResult contact;
//            float ax = tA.position.x, ay = tA.position.y;
//            float bx = tB.position.x, by = tB.position.y;
//
//            if (A.shape == SimShapeType::Circle && B.shape == SimShapeType::Circle)
//                contact = CircleVsCircle(ax, ay, A.hw, bx, by, B.hw);
//            else if (A.shape == SimShapeType::Circle && B.shape == SimShapeType::Rect)
//                contact = CircleVsAABB(ax, ay, A.hw, bx, by, B.hw, B.hh);
//            else if (A.shape == SimShapeType::Rect && B.shape == SimShapeType::Circle)
//            {
//                contact = CircleVsAABB(bx, by, B.hw, ax, ay, A.hw, A.hh);
//                // normal is circle→AABB, flip for our convention
//                contact.nx = -contact.nx;
//                contact.ny = -contact.ny;
//            }
//            else
//                contact = AABBvsAABB(ax, ay, A.hw, A.hh, bx, by, B.hw, B.hh);
//
//            if (!contact.hit) continue;
//
//            // positional correction
//            float invMassA = A.isStatic ? 0.0f : (A.mass > 0 ? 1.0f / A.mass : 0.0f);
//            float invMassB = B.isStatic ? 0.0f : (B.mass > 0 ? 1.0f / B.mass : 0.0f);
//            float totalInv = invMassA + invMassB;
//            if (totalInv < 0.0001f) continue;
//
//            const float SLOP = 0.5f;
//            float corr = std::max(contact.depth - SLOP, 0.0f) / totalInv;
//            tA.position.x += contact.nx * corr * invMassA;
//            tA.position.y += contact.ny * corr * invMassA;
//            tB.position.x -= contact.nx * corr * invMassB;
//            tB.position.y -= contact.ny * corr * invMassB;
//
//            // impulse
//            float relVx = A.vx - B.vx;
//            float relVy = A.vy - B.vy;
//            float relVn = relVx * contact.nx + relVy * contact.ny;
//            if (relVn > 0) continue;   // separating
//
//            float e = std::min(A.restitution, B.restitution);
//            float j = -(1.0f + e) * relVn / totalInv;
//
//            A.vx += j * invMassA * contact.nx;
//            A.vy += j * invMassA * contact.ny;
//            B.vx -= j * invMassB * contact.nx;
//            B.vy -= j * invMassB * contact.ny;
//        }
//    }
//}
//
//// ─────────────────────────────────────────────────────────────────────────────
////  Snapshot / reset
//// ─────────────────────────────────────────────────────────────────────────────
//void PhysSim::SnapshotInitial()
//{
//    for (auto e : _bodies)
//    {
//        if (!ecs.Alive(e)) continue;
//        SimBody& b = ecs.Get<SimBody>(e);
//        const TransformComponent& t = ecs.Get<TransformComponent>(e);
//        b.initX = t.position.x;
//        b.initY = t.position.y;
//        b.initVx = b.vx;
//        b.initVy = b.vy;
//    }
//}
//
//void PhysSim::ResetToInitial()
//{
//    for (auto e : _bodies)
//    {
//        if (!ecs.Alive(e)) continue;
//        SimBody& b = ecs.Get<SimBody>(e);
//        TransformComponent& t = ecs.Get<TransformComponent>(e);
//        t.position.x = b.initX;
//        t.position.y = b.initY;
//        b.vx = b.initVx;
//        b.vy = b.initVy;
//        b.ax = 0.0f;
//        b.ay = 0.0f;
//    }
//}
//
//// ─────────────────────────────────────────────────────────────────────────────
////  Spawn
//// ─────────────────────────────────────────────────────────────────────────────
//ECS::Entity PhysSim::SpawnBody(SimShapeType shape, float x, float y)
//{
//    ECS::Entity e = ecs.Create();
//
//    SimBody b;
//    b.shape = shape;
//    b.hw = (shape == SimShapeType::Circle) ? 30.0f : 40.0f;
//    b.hh = 30.0f;
//    b.mass = 1.0f;
//    b.isStatic = false;
//    b.initX = x;
//    b.initY = y;
//
//    TransformComponent t;
//    t.position = { x, y };
//    t.scale = { 1.0f, 1.0f };
//
//    ecs.Add<SimBody>(e, b);
//    ecs.Add<TransformComponent>(e, t);
//
//    _bodies.push_back(e);
//    return e;
//}
//
//// ─────────────────────────────────────────────────────────────────────────────
////  Delete
//// ─────────────────────────────────────────────────────────────────────────────
//void PhysSim::DeleteSelectedBody()
//{
//    // remove attached forces
//    _forces.erase(
//        std::remove_if(_forces.begin(), _forces.end(),
//            [&](const SimForce& sf) { return sf.target == _selectedBody; }),
//        _forces.end());
//
//    ecs.Destroy(_selectedBody);
//    _bodies.erase(
//        std::remove(_bodies.begin(), _bodies.end(), _selectedBody),
//        _bodies.end());
//    _selectedBody = 0;
//    _selectedForce = -1;
//}
//
//void PhysSim::DeleteSelectedForce()
//{
//    if (_selectedForce < 0 || _selectedForce >= (int)_forces.size()) return;
//    _forces.erase(_forces.begin() + _selectedForce);
//    _selectedForce = -1;
//}
//
//// ─────────────────────────────────────────────────────────────────────────────
////  Hit-testing
//// ─────────────────────────────────────────────────────────────────────────────
//ECS::Entity PhysSim::HitTestBody(float x, float y) const
//{
//    for (auto e : _bodies)
//    {
//        if (!ecs.Alive(e)) continue;
//        const SimBody& b = ecs.Get<SimBody>(e);
//        const TransformComponent& t = ecs.Get<TransformComponent>(e);
//        float dx = x - t.position.x;
//        float dy = y - t.position.y;
//        if (b.shape == SimShapeType::Circle)
//        {
//            if (dx * dx + dy * dy <= b.hw * b.hw)
//                return e;
//        }
//        else
//        {
//            if (std::abs(dx) <= b.hw && std::abs(dy) <= b.hh)
//                return e;
//        }
//    }
//    return 0;
//}
//
//// Point-to-segment distance for force arrow hit-testing
//static float PointSegDist(float px, float py,
//    float ax, float ay,
//    float bx, float by)
//{
//    float dx = bx - ax, dy = by - ay;
//    float lenSq = dx * dx + dy * dy;
//    if (lenSq < 0.0001f)
//    {
//        float ex = px - ax, ey = py - ay;
//        return std::sqrt(ex * ex + ey * ey);
//    }
//    float t = std::max(0.0f, std::min(1.0f, ((px - ax) * dx + (py - ay) * dy) / lenSq));
//    float projX = ax + t * dx - px;
//    float projY = ay + t * dy - py;
//    return std::sqrt(projX * projX + projY * projY);
//}
//
//int PhysSim::HitTestForce(float x, float y) const
//{
//    for (int i = (int)_forces.size() - 1; i >= 0; i--)
//    {
//        const SimForce& sf = _forces[i];
//        if (!ecs.Alive(sf.target)) continue;
//        float ex = sf.anchorX + sf.fx * FORCE_SCALE;
//        float ey = sf.anchorY + sf.fy * FORCE_SCALE;
//        float dist = PointSegDist(x, y, sf.anchorX, sf.anchorY, ex, ey);
//        if (dist <= HIT_MARGIN)
//            return i;
//    }
//    return -1;
//}
//
//// ─────────────────────────────────────────────────────────────────────────────
////  Rendering helpers
//// ─────────────────────────────────────────────────────────────────────────────
//void PhysSim::DrawCircle(float cx, float cy, float r) const
//{
//    // Triangle-fan via SDL_RenderGeometry
//    SDL_Renderer* rend = _data->SDLrenderer;
//    constexpr int SEGS = 32;
//    SDL_Vertex verts[SEGS + 2];
//    int indices[SEGS * 3];
//
//    Uint8 R, G, B, A;
//    SDL_GetRenderDrawColor(rend, &R, &G, &B, &A);
//    SDL_FColor col = { R / 255.0f, G / 255.0f, B / 255.0f, A / 255.0f };
//
//    verts[0] = { {cx, cy}, col, {0,0} };
//    for (int i = 0; i <= SEGS; i++)
//    {
//        float angle = (float)i / SEGS * 2.0f * 3.14159265f;
//        verts[i + 1] = { {cx + cosf(angle) * r, cy + sinf(angle) * r}, col, {0,0} };
//    }
//    for (int i = 0; i < SEGS; i++)
//    {
//        indices[i * 3 + 0] = 0;
//        indices[i * 3 + 1] = i + 1;
//        indices[i * 3 + 2] = i + 2;
//    }
//    SDL_RenderGeometry(rend, nullptr,
//        verts, SEGS + 2,
//        indices, SEGS * 3);
//}
//
//void PhysSim::DrawBody(const SimBody& b, float x, float y) const
//{
//    SDL_Renderer* r = _data->SDLrenderer;
//    bool sel = b.selected;
//
//    if (b.isStatic)
//        SDL_SetRenderDrawColor(r, 120, 120, 130, 255);
//    else if (sel)
//        SDL_SetRenderDrawColor(r, 255, 220, 80, 255);
//    else if (b.shape == SimShapeType::Circle)
//        SDL_SetRenderDrawColor(r, 100, 180, 255, 255);
//    else
//        SDL_SetRenderDrawColor(r, 100, 220, 140, 255);
//
//    if (b.shape == SimShapeType::Circle)
//    {
//        DrawCircle(x, y, b.hw);
//        // outline
//        SDL_SetRenderDrawColor(r, 255, 255, 255, sel ? 255 : 80);
//        // draw outline by drawing a slightly larger circle outline (approx with rect border)
//        constexpr int SEGS = 32;
//        for (int i = 0; i < SEGS; i++)
//        {
//            float a0 = (float)i / SEGS * 2.0f * 3.14159265f;
//            float a1 = (float)(i + 1) / SEGS * 2.0f * 3.14159265f;
//            SDL_RenderLine(r,
//                x + cosf(a0) * b.hw, y + sinf(a0) * b.hw,
//                x + cosf(a1) * b.hw, y + sinf(a1) * b.hw);
//        }
//    }
//    else
//    {
//        SDL_FRect rect = { x - b.hw, y - b.hh, b.hw * 2.0f, b.hh * 2.0f };
//        SDL_RenderFillRect(r, &rect);
//        SDL_SetRenderDrawColor(r, 255, 255, 255, sel ? 255 : 80);
//        SDL_RenderRect(r, &rect);
//    }
//}
//
//void PhysSim::DrawForceArrow(float x0, float y0, float x1, float y1,
//    bool selected, bool preview) const
//{
//    SDL_Renderer* r = _data->SDLrenderer;
//
//    if (preview)
//        SDL_SetRenderDrawColor(r, 255, 220, 80, 180);
//    else if (selected)
//        SDL_SetRenderDrawColor(r, 255, 80, 80, 255);
//    else
//        SDL_SetRenderDrawColor(r, 255, 160, 40, 200);
//
//    // shaft
//    SDL_RenderLine(r, x0, y0, x1, y1);
//
//    // arrowhead
//    float dx = x1 - x0, dy = y1 - y0;
//    float len = std::sqrt(dx * dx + dy * dy);
//    if (len < 1.0f) return;
//    dx /= len; dy /= len;
//
//    const float HEAD = 12.0f, HALF = 5.0f;
//    float lx = x1 - dx * HEAD - dy * HALF;
//    float ly = y1 - dy * HEAD + dx * HALF;
//    float rx2 = x1 - dx * HEAD + dy * HALF;
//    float ry2 = y1 - dy * HEAD - dx * HALF;
//
//    SDL_RenderLine(r, x1, y1, lx, ly);
//    SDL_RenderLine(r, x1, y1, rx2, ry2);
//
//    // draw a small filled circle at the anchor point
//    SDL_FRect dot = { x0 - 4.0f, y0 - 4.0f, 8.0f, 8.0f };
//    SDL_RenderFillRect(r, &dot);
//}
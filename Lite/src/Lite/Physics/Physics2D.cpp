#include <Lite/Scene/Scene.h>

#include <Lite/Scene/Console.h>
#include <Lite/Script/ScriptModule.h>

#include <box2d/box2d.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>

namespace Lite {

	struct Scene::PhysicsStorage
	{
		b2WorldId World = {};

		struct Body
		{
			uint32_t Entity = 0;
			b2BodyId Id = {};
			Vec3 Position {};
			float Rotation = 0.0f;
			Vec2 LinearVelocity {};
			float AngularVelocity = 0.0f;
			bool HasRigidbody = false;
		};

		std::vector<Body> Bodies;
	};

	namespace {

		b2BodyType ToBodyType(BodyType type)
		{
			switch (type)
			{
				case BodyType::Static: return b2_staticBody;
				case BodyType::Kinematic: return b2_kinematicBody;
				default: return b2_dynamicBody;
			}
		}

		b2ShapeDef MakeShape(bool trigger)
		{
			b2ShapeDef shape = b2DefaultShapeDef();
			shape.density = trigger ? 0.0f : 1.0f;
			shape.isSensor = trigger;
			shape.enableContactEvents = !trigger;
			// A sensor only reports a visitor that also has sensor events enabled.
			shape.enableSensorEvents = true;
			shape.material.friction = 0.5f;
			shape.material.restitution = 0.15f;
			return shape;
		}

		uint32_t EntityId(b2ShapeId shape)
		{
			if (!b2Shape_IsValid(shape))
				return 0;

			b2BodyId body = b2Shape_GetBody(shape);
			if (!b2Body_IsValid(body))
				return 0;

			return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(b2Body_GetUserData(body)));
		}

		std::string EntityName(Scene& scene, uint32_t id)
		{
			Entity entity = scene.GetEntity(id);
			if (!entity || entity.GetName().empty())
				return id == 0 ? "unknown" : "entity";
			return entity.GetName();
		}

		void ReportOverlap(Scene& scene, b2ShapeId first, b2ShapeId second, const char* label, bool begin)
		{
			if (!b2Shape_IsValid(first) || !b2Shape_IsValid(second))
				return;

			uint32_t firstId = EntityId(first);
			uint32_t secondId = EntityId(second);
			Console::Log(std::format("{}: {} and {}", label, EntityName(scene, firstId), EntityName(scene, secondId)));
			ScriptRuntime::OnCollision(scene, firstId, secondId, begin);
		}

		bool AddBox(b2BodyId body, const Transform& transform, const BoxCollider2DComponent& box)
		{
			float halfWidth = std::fabs(transform.Scale.x) * box.Size.x * 0.5f;
			float halfHeight = std::fabs(transform.Scale.y) * box.Size.y * 0.5f;
			if (halfWidth < 0.01f)
				halfWidth = 0.01f;
			if (halfHeight < 0.01f)
				halfHeight = 0.01f;

			b2Vec2 center { box.Offset.x * transform.Scale.x, box.Offset.y * transform.Scale.y };
			b2Polygon polygon = b2MakeOffsetBox(halfWidth, halfHeight, center, b2MakeRot(0.0f));
			b2ShapeDef shape = MakeShape(box.IsTrigger);
			b2CreatePolygonShape(body, &shape, &polygon);
			return !box.IsTrigger;
		}

		bool AddCircle(b2BodyId body, const Transform& transform, const CircleCollider2DComponent& circle)
		{
			float scale = std::fabs(transform.Scale.x);
			float other = std::fabs(transform.Scale.y);
			if (other > scale)
				scale = other;

			b2Circle shape {};
			shape.center = { circle.Offset.x * transform.Scale.x, circle.Offset.y * transform.Scale.y };
			shape.radius = circle.Radius * scale;
			if (shape.radius < 0.01f)
				shape.radius = 0.01f;

			b2ShapeDef definition = MakeShape(circle.IsTrigger);
			b2CreateCircleShape(body, &definition, &shape);
			return !circle.IsTrigger;
		}

		void ApplyMass(b2BodyId body, const Rigidbody2DComponent& rigidbody, bool solid)
		{
			if (rigidbody.Type != BodyType::Dynamic || rigidbody.Mass <= 0.0f)
				return;

			if (!solid)
			{
				b2MassData mass {};
				mass.mass = rigidbody.Mass;
				mass.rotationalInertia = 0.2f * rigidbody.Mass;
				b2Body_SetMassData(body, mass);
				return;
			}

			b2MassData mass = b2Body_GetMassData(body);
			if (mass.mass <= 0.0f)
				return;

			float scale = rigidbody.Mass / mass.mass;
			mass.mass = rigidbody.Mass;
			mass.rotationalInertia *= scale;
			b2Body_SetMassData(body, mass);
		}

	}

	void Scene::StartPhysics()
	{
		StopPhysics();

		constexpr float kGravity = 9.81f;
		b2WorldDef worldDef = b2DefaultWorldDef();
		worldDef.gravity = { 0.0f, -kGravity };
		m_Physics = new PhysicsStorage();
		m_Physics->World = b2CreateWorld(&worldDef);

		for (Entity entity : GetEntities())
			SpawnPhysicsBody(entity, false);

		Console::Log(std::format("Box2D started: {} bodies", m_Physics->Bodies.size()));
		ScriptRuntime::Load(*this);
	}

	void Scene::SpawnPhysicsBody(Entity entity, bool preserveSnapshot)
	{
		if (m_Physics == nullptr || !b2World_IsValid(m_Physics->World) || !entity)
			return;

		bool keepSnapshot = false;
		Vec3 position {};
		float rotation = 0.0f;
		Vec2 linear {};
		float angular = 0.0f;
		if (preserveSnapshot)
		{
			for (auto body = m_Physics->Bodies.begin(); body != m_Physics->Bodies.end(); ++body)
			{
				if (body->Entity != entity.GetId())
					continue;

				keepSnapshot = true;
				position = body->Position;
				rotation = body->Rotation;
				linear = body->LinearVelocity;
				angular = body->AngularVelocity;
				if (b2Body_IsValid(body->Id))
					b2DestroyBody(body->Id);
				m_Physics->Bodies.erase(body);
				break;
			}
		}

		TransformComponent* transformComponent = entity.Get<TransformComponent>();
		if (transformComponent == nullptr)
			return;

		Rigidbody2DComponent* rigidbody = entity.Get<Rigidbody2DComponent>();
		BoxCollider2DComponent* box = entity.Get<BoxCollider2DComponent>();
		CircleCollider2DComponent* circle = entity.Get<CircleCollider2DComponent>();
		SpinComponent* spin = entity.Get<SpinComponent>();
		if (rigidbody == nullptr && box == nullptr && circle == nullptr)
			return;

		const Transform& local = transformComponent->Local;
		const Transform world = WorldTransform(entity.GetId());
		PhysicsStorage::Body stored {};
		stored.Entity = entity.GetId();
		stored.Position = keepSnapshot ? position : local.Position;
		stored.Rotation = keepSnapshot ? rotation : local.GetRotationZ();
		stored.HasRigidbody = rigidbody != nullptr;
		if (rigidbody != nullptr)
		{
			stored.LinearVelocity = keepSnapshot ? linear : rigidbody->LinearVelocity;
			stored.AngularVelocity = keepSnapshot ? angular : rigidbody->AngularVelocity;
		}

		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.type = rigidbody != nullptr ? ToBodyType(rigidbody->Type) : b2_staticBody;
		bodyDef.position = { world.Position.x, world.Position.y };
		bodyDef.rotation = b2MakeRot(world.GetRotationZ());
		bodyDef.fixedRotation = rigidbody != nullptr && rigidbody->FreezeRotation;
		bodyDef.gravityScale = rigidbody != nullptr ? rigidbody->GravityScale : 1.0f;
		bodyDef.isBullet = bodyDef.type == b2_dynamicBody;
		bodyDef.userData = reinterpret_cast<void*>(static_cast<uintptr_t>(entity.GetId()));
		if (rigidbody != nullptr)
		{
			bodyDef.linearVelocity = { rigidbody->LinearVelocity.x, rigidbody->LinearVelocity.y };
			bodyDef.angularVelocity = rigidbody->AngularVelocity;
		}
		if (spin != nullptr && !bodyDef.fixedRotation)
			bodyDef.angularVelocity += spin->Rate;

		stored.Id = b2CreateBody(m_Physics->World, &bodyDef);
		std::string name = entity.GetName();
		b2Body_SetName(stored.Id, name.c_str());

		bool solid = false;
		if (box != nullptr)
			solid = AddBox(stored.Id, world, *box) || solid;
		if (circle != nullptr)
			solid = AddCircle(stored.Id, world, *circle) || solid;
		if (rigidbody != nullptr)
			ApplyMass(stored.Id, *rigidbody, solid);

		m_Physics->Bodies.push_back(stored);
	}

	void Scene::RemovePhysicsBody(uint32_t entityId)
	{
		if (m_Physics == nullptr)
			return;

		for (auto body = m_Physics->Bodies.begin(); body != m_Physics->Bodies.end(); ++body)
		{
			if (body->Entity != entityId)
				continue;

			if (b2Body_IsValid(body->Id))
				b2DestroyBody(body->Id);
			m_Physics->Bodies.erase(body);
			return;
		}
	}

	void Scene::RefreshPhysics(uint32_t entityId)
	{
		if (!GetEntity(entityId))
			return;

		std::vector<uint32_t> ids { entityId };
		for (size_t index = 0; index < ids.size(); ++index)
		{
			for (const Record& record : m_Records)
			{
				if (record.Parent == ids[index])
					ids.push_back(record.Id);
			}
		}

		for (uint32_t id : ids)
			SpawnPhysicsBody(GetEntity(id), true);
	}

	void Scene::StopPhysics()
	{
		ScriptRuntime::Unload();
		if (m_Physics == nullptr)
			return;

		for (const PhysicsStorage::Body& body : m_Physics->Bodies)
		{
			Entity entity = GetEntity(body.Entity);
			TransformComponent* transform = entity.Get<TransformComponent>();
			if (transform == nullptr)
				continue;

			transform->Local.Position = body.Position;
			transform->Local.SetRotationZ(body.Rotation);
			if (body.HasRigidbody)
			{
				if (Rigidbody2DComponent* rigidbody = entity.Get<Rigidbody2DComponent>())
				{
					rigidbody->LinearVelocity = body.LinearVelocity;
					rigidbody->AngularVelocity = body.AngularVelocity;
				}
			}
		}

		if (b2World_IsValid(m_Physics->World))
			b2DestroyWorld(m_Physics->World);

		delete m_Physics;
		m_Physics = nullptr;
	}

	void Scene::SyncPhysics()
	{
		if (m_Physics == nullptr)
			return;

		for (const PhysicsStorage::Body& body : m_Physics->Bodies)
		{
			Entity entity = GetEntity(body.Entity);
			TransformComponent* transformComponent = entity.Get<TransformComponent>();
			if (transformComponent == nullptr || !b2Body_IsValid(body.Id))
				continue;

			const Transform world = WorldTransform(body.Entity);
			b2Body_SetTransform(body.Id, { world.Position.x, world.Position.y }, b2MakeRot(world.GetRotationZ()));
			if (Rigidbody2DComponent* rigidbody = entity.Get<Rigidbody2DComponent>())
			{
				b2Body_SetLinearVelocity(body.Id, { rigidbody->LinearVelocity.x, rigidbody->LinearVelocity.y });
				float angular = rigidbody->AngularVelocity;
				SpinComponent* spin = entity.Get<SpinComponent>();
				if (spin != nullptr && !rigidbody->FreezeRotation)
					angular += spin->Rate;
				b2Body_SetAngularVelocity(body.Id, angular);
			}
		}
	}

	void Scene::StepPhysics(float seconds)
	{
		if (m_Physics == nullptr || !b2World_IsValid(m_Physics->World))
			return;

		if (seconds < 0.0f)
			return;
		if (seconds > 0.05f)
			seconds = 0.05f;

		for (const PhysicsStorage::Body& body : m_Physics->Bodies)
		{
			if (!b2Body_IsValid(body.Id))
				continue;

			Entity entity = GetEntity(body.Entity);
			Rigidbody2DComponent* rigidbody = entity.Get<Rigidbody2DComponent>();
			if (rigidbody == nullptr || rigidbody->Type == BodyType::Static)
				continue;

			b2Body_SetLinearVelocity(body.Id, { rigidbody->LinearVelocity.x, rigidbody->LinearVelocity.y });
		}

		b2World_Step(m_Physics->World, seconds, 4);

		b2ContactEvents contacts = b2World_GetContactEvents(m_Physics->World);
		for (int index = 0; index < contacts.beginCount; ++index)
			ReportOverlap(*this, contacts.beginEvents[index].shapeIdA, contacts.beginEvents[index].shapeIdB, "Collision", true);
		for (int index = 0; index < contacts.endCount; ++index)
			ReportOverlap(*this, contacts.endEvents[index].shapeIdA, contacts.endEvents[index].shapeIdB, "Collision end", false);

		b2SensorEvents sensors = b2World_GetSensorEvents(m_Physics->World);
		for (int index = 0; index < sensors.beginCount; ++index)
			ReportOverlap(*this, sensors.beginEvents[index].sensorShapeId, sensors.beginEvents[index].visitorShapeId, "Trigger", true);
		for (int index = 0; index < sensors.endCount; ++index)
			ReportOverlap(*this, sensors.endEvents[index].sensorShapeId, sensors.endEvents[index].visitorShapeId, "Trigger end", false);

		std::vector<size_t> order(m_Physics->Bodies.size());
		for (size_t index = 0; index < order.size(); ++index)
			order[index] = index;
		std::stable_sort(order.begin(), order.end(), [&](size_t left, size_t right)
		{
			return Depth(m_Physics->Bodies[left].Entity) < Depth(m_Physics->Bodies[right].Entity);
		});

		for (size_t index : order)
		{
			const PhysicsStorage::Body& body = m_Physics->Bodies[index];
			Entity entity = GetEntity(body.Entity);
			TransformComponent* transform = entity.Get<TransformComponent>();
			if (transform == nullptr || !b2Body_IsValid(body.Id))
				continue;

			Rigidbody2DComponent* rigidbody = entity.Get<Rigidbody2DComponent>();
			const bool simulated = rigidbody != nullptr && rigidbody->Type != BodyType::Static;
			if (simulated)
			{
				b2Vec2 position = b2Body_GetPosition(body.Id);
				SetLocalPose(body.Entity, position.x, position.y, b2Rot_GetAngle(b2Body_GetRotation(body.Id)));
			}

			if (!body.HasRigidbody || rigidbody == nullptr)
				continue;

			b2Vec2 velocity = b2Body_GetLinearVelocity(body.Id);
			rigidbody->LinearVelocity = { velocity.x, velocity.y };
			rigidbody->AngularVelocity = b2Body_GetAngularVelocity(body.Id);
		}

		for (const PhysicsStorage::Body& body : m_Physics->Bodies)
		{
			if (GetParent(body.Entity) == 0 || !b2Body_IsValid(body.Id))
				continue;

			Entity entity = GetEntity(body.Entity);
			Rigidbody2DComponent* rigidbody = entity.Get<Rigidbody2DComponent>();
			if (rigidbody != nullptr && rigidbody->Type != BodyType::Static)
				continue;

			const Transform world = WorldTransform(body.Entity);
			b2Vec2 position = b2Body_GetPosition(body.Id);
			float angle = b2Rot_GetAngle(b2Body_GetRotation(body.Id));
			if (std::fabs(position.x - world.Position.x) < 0.0001f
				&& std::fabs(position.y - world.Position.y) < 0.0001f
				&& std::fabs(angle - world.GetRotationZ()) < 0.0001f)
				continue;

			b2Body_SetTransform(body.Id, { world.Position.x, world.Position.y }, b2MakeRot(world.GetRotationZ()));
		}
	}

}

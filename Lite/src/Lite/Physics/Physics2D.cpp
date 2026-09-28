#include <Lite/Scene/Scene.h>

#include <Lite/Scene/Console.h>

#include <box2d/box2d.h>

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
			shape.material.friction = 0.5f;
			shape.material.restitution = 0.15f;
			return shape;
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

		for (Record& record : m_Records)
		{
			if (!record.Transform)
				continue;
			if (!record.Rigidbody2D && !record.BoxCollider2D && !record.CircleCollider2D)
				continue;

			const Transform& transform = record.Transform->Local;
			PhysicsStorage::Body stored {};
			stored.Entity = record.Id;
			stored.Position = transform.Position;
			stored.Rotation = transform.GetRotationZ();
			stored.HasRigidbody = record.Rigidbody2D.has_value();
			if (record.Rigidbody2D)
			{
				stored.LinearVelocity = record.Rigidbody2D->LinearVelocity;
				stored.AngularVelocity = record.Rigidbody2D->AngularVelocity;
			}

			b2BodyDef bodyDef = b2DefaultBodyDef();
			bodyDef.type = record.Rigidbody2D ? ToBodyType(record.Rigidbody2D->Type) : b2_staticBody;
			bodyDef.position = { transform.Position.x, transform.Position.y };
			bodyDef.rotation = b2MakeRot(transform.GetRotationZ());
			bodyDef.fixedRotation = record.Rigidbody2D && record.Rigidbody2D->FreezeRotation;
			bodyDef.gravityScale = record.Rigidbody2D ? record.Rigidbody2D->GravityScale : 1.0f;
			bodyDef.isBullet = bodyDef.type == b2_dynamicBody;
			bodyDef.userData = reinterpret_cast<void*>(static_cast<uintptr_t>(record.Id));
			if (record.Rigidbody2D)
			{
				bodyDef.linearVelocity = { record.Rigidbody2D->LinearVelocity.x, record.Rigidbody2D->LinearVelocity.y };
				bodyDef.angularVelocity = record.Rigidbody2D->AngularVelocity;
			}
			if (record.Spin && !bodyDef.fixedRotation)
				bodyDef.angularVelocity += record.Spin->Rate;

			stored.Id = b2CreateBody(m_Physics->World, &bodyDef);
			b2Body_SetName(stored.Id, record.Name.c_str());

			bool solid = false;
			if (record.BoxCollider2D)
				solid = AddBox(stored.Id, transform, *record.BoxCollider2D) || solid;
			if (record.CircleCollider2D)
				solid = AddCircle(stored.Id, transform, *record.CircleCollider2D) || solid;
			if (record.Rigidbody2D)
				ApplyMass(stored.Id, *record.Rigidbody2D, solid);

			m_Physics->Bodies.push_back(stored);
		}

		Console::Log(std::format("Box2D started: {} bodies", m_Physics->Bodies.size()));
	}

	void Scene::StopPhysics()
	{
		if (m_Physics == nullptr)
			return;

		for (const PhysicsStorage::Body& body : m_Physics->Bodies)
		{
			Record* record = FindRecord(body.Entity);
			if (record == nullptr || !record->Transform)
				continue;

			record->Transform->Local.Position = body.Position;
			record->Transform->Local.SetRotationZ(body.Rotation);
			if (body.HasRigidbody && record->Rigidbody2D)
			{
				record->Rigidbody2D->LinearVelocity = body.LinearVelocity;
				record->Rigidbody2D->AngularVelocity = body.AngularVelocity;
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
			Record* record = FindRecord(body.Entity);
			if (record == nullptr || !record->Transform || !b2Body_IsValid(body.Id))
				continue;

			const Transform& transform = record->Transform->Local;
			b2Body_SetTransform(body.Id, { transform.Position.x, transform.Position.y }, b2MakeRot(transform.GetRotationZ()));
			if (record->Rigidbody2D)
			{
				b2Body_SetLinearVelocity(body.Id, { record->Rigidbody2D->LinearVelocity.x, record->Rigidbody2D->LinearVelocity.y });
				float angular = record->Rigidbody2D->AngularVelocity;
				if (record->Spin && !record->Rigidbody2D->FreezeRotation)
					angular += record->Spin->Rate;
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

		b2World_Step(m_Physics->World, seconds, 4);

		b2ContactEvents contacts = b2World_GetContactEvents(m_Physics->World);
		for (int index = 0; index < contacts.beginCount; ++index)
		{
			b2BodyId first = b2Shape_GetBody(contacts.beginEvents[index].shapeIdA);
			b2BodyId second = b2Shape_GetBody(contacts.beginEvents[index].shapeIdB);
			auto nameOf = [this](b2BodyId body)
			{
				if (!b2Body_IsValid(body))
					return std::string("unknown");
				auto id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(b2Body_GetUserData(body)));
				Record* record = FindRecord(id);
				if (record == nullptr || record->Name.empty())
					return std::string("entity");
				return record->Name;
			};
			Console::Log(std::format("Collision: {} and {}", nameOf(first), nameOf(second)));
		}

		for (const PhysicsStorage::Body& body : m_Physics->Bodies)
		{
			Record* record = FindRecord(body.Entity);
			if (record == nullptr || !record->Transform || !b2Body_IsValid(body.Id))
				continue;

			b2Vec2 position = b2Body_GetPosition(body.Id);
			record->Transform->Local.Position.x = position.x;
			record->Transform->Local.Position.y = position.y;
			record->Transform->Local.SetRotationZ(b2Rot_GetAngle(b2Body_GetRotation(body.Id)));

			if (!body.HasRigidbody || !record->Rigidbody2D)
				continue;

			b2Vec2 velocity = b2Body_GetLinearVelocity(body.Id);
			record->Rigidbody2D->LinearVelocity = { velocity.x, velocity.y };
			record->Rigidbody2D->AngularVelocity = b2Body_GetAngularVelocity(body.Id);
		}
	}

}

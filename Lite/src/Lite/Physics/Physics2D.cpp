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

		struct Joint
		{
			uint32_t Owner = 0;
			ComponentId Kind = ComponentId::DistanceJoint2D;
			b2JointId Id = {};
		};

		std::vector<Body> Bodies;
		std::vector<Joint> Joints;
	};

	void Scene::PhysicsDeleter::operator()(PhysicsStorage* storage) const
	{
		delete storage;
	}

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

		b2ShapeDef MakeShape(bool trigger, uint64_t layer, uint64_t mask, float friction, float restitution)
		{
			b2ShapeDef shape = b2DefaultShapeDef();
			shape.density = trigger ? 0.0f : 1.0f;
			shape.isSensor = trigger;
			shape.enableContactEvents = !trigger;
			// A sensor only reports a visitor that also has sensor events enabled.
			shape.enableSensorEvents = true;
			shape.material.friction = friction < 0.0f ? 0.0f : friction;
			shape.material.restitution = restitution < 0.0f ? 0.0f : restitution;
			shape.filter.categoryBits = layer;
			shape.filter.maskBits = mask;
			shape.filter.groupIndex = 0;
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
			b2ShapeDef shape = MakeShape(box.IsTrigger, box.Layer, box.Mask, box.Friction, box.Restitution);
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

			b2ShapeDef definition = MakeShape(circle.IsTrigger, circle.Layer, circle.Mask, circle.Friction, circle.Restitution);
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

		b2WorldDef worldDef = b2DefaultWorldDef();
		worldDef.gravity = { m_Gravity.x, m_Gravity.y };
		m_Physics.reset(new PhysicsStorage());
		m_Physics->World = b2CreateWorld(&worldDef);

		for (Entity entity : GetEntities())
			SpawnPhysicsBody(entity, false);

		SpawnDistanceJoints();
		SpawnRevoluteJoints();

		m_Motion.clear();
		m_StepRemainder = 0.0f;
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

		RespawnDistanceJoints(ids);
		RespawnRevoluteJoints(ids);
	}

	struct Scene::PhysicsBodyRef
	{
		b2BodyId Id = {};
	};

	bool Scene::TakeBody(uint32_t entityId, PhysicsBodyRef& body) const
	{
		body.Id = {};
		if (m_Physics == nullptr)
			return false;

		for (const PhysicsStorage::Body& stored : m_Physics->Bodies)
		{
			if (stored.Entity == entityId && b2Body_IsValid(stored.Id))
			{
				body.Id = stored.Id;
				return true;
			}
		}

		return false;
	}

	void Scene::ApplyForce(uint32_t id, Vec2 force)
	{
		if (m_Playback != ScenePlayback::Playing)
			return;

		PhysicsBodyRef body;
		if (!TakeBody(id, body))
			return;

		b2Body_ApplyForceToCenter(body.Id, { force.x, force.y }, true);
	}

	void Scene::ApplyLinearImpulse(uint32_t id, Vec2 impulse)
	{
		if (m_Playback != ScenePlayback::Playing)
			return;

		PhysicsBodyRef body;
		if (!TakeBody(id, body))
			return;

		b2Body_ApplyLinearImpulseToCenter(body.Id, { impulse.x, impulse.y }, true);
		if (Rigidbody2DComponent* rigidbody = GetEntity(id).Get<Rigidbody2DComponent>())
		{
			b2Vec2 velocity = b2Body_GetLinearVelocity(body.Id);
			rigidbody->LinearVelocity = { velocity.x, velocity.y };
		}
	}

	void Scene::StopPhysics()
	{
		m_Motion.clear();
		m_StepRemainder = 0.0f;
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

		m_Physics.reset();
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

	void Scene::CaptureMotion()
	{
		m_Motion.clear();
		if (m_Physics == nullptr)
			return;

		for (const PhysicsStorage::Body& body : m_Physics->Bodies)
		{
			Entity entity = GetEntity(body.Entity);
			Rigidbody2DComponent* rigidbody = entity.Get<Rigidbody2DComponent>();
			if (rigidbody == nullptr || rigidbody->Type == BodyType::Static)
				continue;

			TransformComponent* transform = entity.Get<TransformComponent>();
			if (transform == nullptr)
				continue;

			MotionSample sample;
			sample.Entity = body.Entity;
			sample.X = transform->Local.Position.x;
			sample.Y = transform->Local.Position.y;
			sample.Angle = transform->Local.GetRotationZ();
			m_Motion.push_back(sample);
		}
	}

	void Scene::StepPhysics(float seconds)
	{
		if (m_Physics == nullptr || !b2World_IsValid(m_Physics->World))
			return;

		if (seconds <= 0.0f)
			return;

		for (const PhysicsStorage::Body& body : m_Physics->Bodies)
		{
			if (!b2Body_IsValid(body.Id))
				continue;

			Entity entity = GetEntity(body.Entity);
			Rigidbody2DComponent* rigidbody = entity.Get<Rigidbody2DComponent>();
			if (rigidbody == nullptr || rigidbody->Type == BodyType::Static)
				continue;

			b2Body_SetLinearVelocity(body.Id, { rigidbody->LinearVelocity.x, rigidbody->LinearVelocity.y });
			b2Body_SetAngularVelocity(body.Id, rigidbody->AngularVelocity);
			b2Body_SetGravityScale(body.Id, rigidbody->GravityScale);
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

	void Scene::DestroyJoint(uint32_t owner, ComponentId kind)
	{
		if (m_Physics == nullptr)
			return;

		for (auto joint = m_Physics->Joints.begin(); joint != m_Physics->Joints.end(); ++joint)
		{
			if (joint->Owner != owner || joint->Kind != kind)
				continue;

			if (b2Joint_IsValid(joint->Id))
				b2DestroyJoint(joint->Id);
			m_Physics->Joints.erase(joint);
			return;
		}
	}

	void Scene::DestroyDistanceJoint(uint32_t owner)
	{
		DestroyJoint(owner, ComponentId::DistanceJoint2D);
	}

	void Scene::DestroyRevoluteJoint(uint32_t owner)
	{
		DestroyJoint(owner, ComponentId::RevoluteJoint2D);
	}

	void Scene::DetachDistanceJoints(uint32_t entityId, const std::string& name)
	{
		std::vector<uint32_t> owners;
		if (!name.empty())
		{
			for (const Record& record : m_Records)
			{
				if (record.Id == entityId)
					continue;

				auto* joint = static_cast<DistanceJoint2DComponent*>(GetComponent(ComponentId::DistanceJoint2D, record.Id));
				if (joint != nullptr && joint->Other == name)
					owners.push_back(record.Id);
			}
		}

		for (uint32_t owner : owners)
			RemoveComponent(ComponentId::DistanceJoint2D, owner);

		DestroyDistanceJoint(entityId);
	}

	void Scene::SpawnDistanceJoint(uint32_t owner)
	{
		if (m_Physics == nullptr || !b2World_IsValid(m_Physics->World))
			return;

		auto* joint = static_cast<DistanceJoint2DComponent*>(GetComponent(ComponentId::DistanceJoint2D, owner));
		if (joint == nullptr)
			return;

		Entity self = GetEntity(owner);
		const std::string name = self ? self.GetName() : std::string("entity");
		if (joint->Other.empty())
		{
			Console::Log(std::format("Distance joint skipped: {} has no other entity", name));
			return;
		}

		Entity other = Find(joint->Other);
		if (!other)
		{
			Console::Log(std::format("Distance joint skipped: {} is missing", joint->Other));
			return;
		}

		if (other.GetId() == owner)
		{
			Console::Log(std::format("Distance joint skipped: {} cannot join itself", name));
			return;
		}

		b2BodyId bodyA {};
		b2BodyId bodyB {};
		for (const PhysicsStorage::Body& stored : m_Physics->Bodies)
		{
			if (!b2Body_IsValid(stored.Id))
				continue;
			if (stored.Entity == owner)
				bodyA = stored.Id;
			if (stored.Entity == other.GetId())
				bodyB = stored.Id;
		}
		if (!b2Body_IsValid(bodyA) || !b2Body_IsValid(bodyB))
		{
			Console::Log(std::format("Distance joint skipped: {} has no body", !b2Body_IsValid(bodyA) ? name : joint->Other));
			return;
		}

		DestroyDistanceJoint(owner);

		b2Vec2 originA = b2Body_GetPosition(bodyA);
		b2Vec2 originB = b2Body_GetPosition(bodyB);
		float length = joint->Length;
		if (length <= 0.0f)
		{
			float dx = originA.x - originB.x;
			float dy = originA.y - originB.y;
			length = std::sqrt(dx * dx + dy * dy);
		}

		if (length <= 0.0f)
		{
			Console::Log(std::format("Distance joint skipped: {} has no length", name));
			return;
		}

		b2DistanceJointDef definition = b2DefaultDistanceJointDef();
		definition.bodyIdA = bodyA;
		definition.bodyIdB = bodyB;
		definition.localAnchorA = { 0.0f, 0.0f };
		definition.localAnchorB = { 0.0f, 0.0f };
		definition.length = length;
		b2JointId created = b2CreateDistanceJoint(m_Physics->World, &definition);
		if (!b2Joint_IsValid(created))
		{
			Console::Log(std::format("Distance joint skipped: {} could not be created", name));
			return;
		}

		PhysicsStorage::Joint stored;
		stored.Owner = owner;
		stored.Kind = ComponentId::DistanceJoint2D;
		stored.Id = created;
		m_Physics->Joints.push_back(stored);
	}

	void Scene::DetachRevoluteJoints(uint32_t entityId, const std::string& name)
	{
		std::vector<uint32_t> owners;
		if (!name.empty())
		{
			for (const Record& record : m_Records)
			{
				if (record.Id == entityId)
					continue;

				auto* joint = static_cast<RevoluteJoint2DComponent*>(GetComponent(ComponentId::RevoluteJoint2D, record.Id));
				if (joint != nullptr && joint->Other == name)
					owners.push_back(record.Id);
			}
		}

		for (uint32_t owner : owners)
			RemoveComponent(ComponentId::RevoluteJoint2D, owner);

		DestroyRevoluteJoint(entityId);
	}

	void Scene::SpawnRevoluteJoint(uint32_t owner)
	{
		if (m_Physics == nullptr || !b2World_IsValid(m_Physics->World))
			return;

		auto* joint = static_cast<RevoluteJoint2DComponent*>(GetComponent(ComponentId::RevoluteJoint2D, owner));
		if (joint == nullptr)
			return;

		Entity self = GetEntity(owner);
		const std::string name = self ? self.GetName() : std::string("entity");
		if (joint->Other.empty())
		{
			Console::Log(std::format("Revolute joint skipped: {} has no other entity", name));
			return;
		}

		Entity other = Find(joint->Other);
		if (!other)
		{
			Console::Log(std::format("Revolute joint skipped: {} is missing", joint->Other));
			return;
		}

		if (other.GetId() == owner)
		{
			Console::Log(std::format("Revolute joint skipped: {} cannot join itself", name));
			return;
		}

		PhysicsBodyRef foundA;
		PhysicsBodyRef foundB;
		if (!TakeBody(owner, foundA) || !TakeBody(other.GetId(), foundB))
		{
			Console::Log(std::format("Revolute joint skipped: {} has no body", b2Body_IsValid(foundA.Id) ? joint->Other : name));
			return;
		}

		b2BodyId bodyA = foundA.Id;
		b2BodyId bodyB = foundB.Id;

		DestroyRevoluteJoint(owner);

		float lower = joint->LowerAngle;
		float upper = joint->UpperAngle;
		if (lower > upper)
			std::swap(lower, upper);
		constexpr float kLimit = 3.1f;
		lower = std::max(-kLimit, std::min(lower, kLimit));
		upper = std::max(-kLimit, std::min(upper, kLimit));

		b2RevoluteJointDef definition = b2DefaultRevoluteJointDef();
		definition.bodyIdA = bodyA;
		definition.bodyIdB = bodyB;
		definition.localAnchorA = { 0.0f, 0.0f };
		definition.localAnchorB = { 0.0f, 0.0f };
		definition.referenceAngle = b2Rot_GetAngle(b2Body_GetRotation(bodyB)) - b2Rot_GetAngle(b2Body_GetRotation(bodyA));
		definition.enableMotor = joint->Motor;
		definition.motorSpeed = joint->MotorSpeed;
		definition.maxMotorTorque = std::max(joint->MaxTorque, 0.0f);
		definition.enableLimit = joint->Limit;
		definition.lowerAngle = lower;
		definition.upperAngle = upper;
		b2JointId created = b2CreateRevoluteJoint(m_Physics->World, &definition);
		if (!b2Joint_IsValid(created))
		{
			Console::Log(std::format("Revolute joint skipped: {} could not be created", name));
			return;
		}

		PhysicsStorage::Joint stored;
		stored.Owner = owner;
		stored.Kind = ComponentId::RevoluteJoint2D;
		stored.Id = created;
		m_Physics->Joints.push_back(stored);
	}

	void Scene::SpawnRevoluteJoints()
	{
		for (const Record& record : m_Records)
		{
			if (GetComponent(ComponentId::RevoluteJoint2D, record.Id) != nullptr)
				SpawnRevoluteJoint(record.Id);
		}
	}

	void Scene::RespawnRevoluteJoints(const std::vector<uint32_t>& ids)
	{
		if (m_Physics == nullptr)
			return;

		auto touches = [&](uint32_t entity)
		{
			for (uint32_t id : ids)
			{
				if (id == entity)
					return true;
			}
			return false;
		};

		std::vector<uint32_t> owners;
		for (const Record& record : m_Records)
		{
			auto* joint = static_cast<RevoluteJoint2DComponent*>(GetComponent(ComponentId::RevoluteJoint2D, record.Id));
			if (joint == nullptr)
				continue;

			Entity other = joint->Other.empty() ? Entity{} : Find(joint->Other);
			if (touches(record.Id) || (other && touches(other.GetId())))
				owners.push_back(record.Id);
		}

		for (uint32_t owner : owners)
			DestroyRevoluteJoint(owner);
		for (uint32_t owner : owners)
			SpawnRevoluteJoint(owner);
	}

	void Scene::SpawnDistanceJoints()
	{
		for (const Record& record : m_Records)
		{
			if (GetComponent(ComponentId::DistanceJoint2D, record.Id) != nullptr)
				SpawnDistanceJoint(record.Id);
		}
	}

	void Scene::RespawnDistanceJoints(const std::vector<uint32_t>& ids)
	{
		if (m_Physics == nullptr)
			return;

		auto touches = [&](uint32_t entity)
		{
			for (uint32_t id : ids)
			{
				if (id == entity)
					return true;
			}
			return false;
		};

		std::vector<uint32_t> owners;
		for (const Record& record : m_Records)
		{
			auto* joint = static_cast<DistanceJoint2DComponent*>(GetComponent(ComponentId::DistanceJoint2D, record.Id));
			if (joint == nullptr)
				continue;

			Entity other = joint->Other.empty() ? Entity{} : Find(joint->Other);
			if (touches(record.Id) || (other && touches(other.GetId())))
				owners.push_back(record.Id);
		}

		for (uint32_t owner : owners)
			DestroyDistanceJoint(owner);
		for (uint32_t owner : owners)
			SpawnDistanceJoint(owner);
	}

	void Scene::SetGravity(Vec2 gravity)
	{
		if (m_Gravity.x == gravity.x && m_Gravity.y == gravity.y)
			return;

		m_Gravity = gravity;
		MarkDirty();
		if (m_Physics != nullptr && b2World_IsValid(m_Physics->World))
			b2World_SetGravity(m_Physics->World, { gravity.x, gravity.y });
	}

	std::vector<uint32_t> Scene::Overlap(Vec2 center, Vec2 halfExtents) const
	{
		std::vector<uint32_t> hits;
		if (m_Physics == nullptr || !b2World_IsValid(m_Physics->World))
			return hits;

		const float halfX = std::fabs(halfExtents.x);
		const float halfY = std::fabs(halfExtents.y);
		b2AABB box;
		box.lowerBound = { center.x - halfX, center.y - halfY };
		box.upperBound = { center.x + halfX, center.y + halfY };

		auto callback = [](b2ShapeId shapeId, void* context) -> bool
		{
			if (!b2Shape_IsValid(shapeId) || b2Shape_IsSensor(shapeId))
				return true;

			const uint32_t entity = EntityId(shapeId);
			if (entity == 0)
				return true;

			auto* found = static_cast<std::vector<uint32_t>*>(context);
			for (uint32_t id : *found)
			{
				if (id == entity)
					return true;
			}

			found->push_back(entity);
			return true;
		};

		b2World_OverlapAABB(m_Physics->World, box, b2DefaultQueryFilter(), callback, &hits);
		return hits;
	}

	std::optional<RayHit> Scene::Raycast(Vec2 origin, Vec2 translation) const
	{
		if (m_Physics == nullptr || !b2World_IsValid(m_Physics->World))
			return std::nullopt;
		if (translation.x == 0.0f && translation.y == 0.0f)
			return std::nullopt;

		struct Query
		{
			bool Hit = false;
			b2ShapeId Shape = {};
			b2Vec2 Point {};
			b2Vec2 Normal {};
			float Fraction = 1.0f;
		} query;

		auto callback = [](b2ShapeId shapeId, b2Vec2 point, b2Vec2 normal, float fraction, void* context) -> float
		{
			if (fraction <= 0.0f || b2Shape_IsSensor(shapeId))
				return -1.0f;

			auto* hit = static_cast<Query*>(context);
			if (hit->Hit && fraction > hit->Fraction)
				return hit->Fraction;

			hit->Hit = true;
			hit->Shape = shapeId;
			hit->Point = point;
			hit->Normal = normal;
			hit->Fraction = fraction;
			return fraction;
		};

		b2World_CastRay(m_Physics->World, { origin.x, origin.y }, { translation.x, translation.y }, b2DefaultQueryFilter(), callback, &query);
		if (!query.Hit)
			return std::nullopt;

		RayHit hit;
		hit.Entity = EntityId(query.Shape);
		hit.Point = { query.Point.x, query.Point.y };
		hit.Normal = { query.Normal.x, query.Normal.y };
		return hit;
	}

}

// Copyright 2018 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_OBJECTS_HEAP_OBJECT_H_
#define V8_OBJECTS_HEAP_OBJECT_H_

#include "src/common/globals.h"
#include "src/objects/instance-type.h"
#include "src/objects/objects.h"
#include "src/objects/tagged-field.h"
#include "src/roots/roots.h"
#include "src/torque/runtime-macro-shims.h"
#include "src/torque/runtime-support.h"

// Has to be the last include (doesn't have include guards):
#include "src/objects/object-macros.h"

namespace v8 {
namespace internal {

class Heap;
class PrimitiveHeapObject;

// HeapObject is the superclass for all classes describing heap allocated
// objects.
class HeapObject : public Object {
 public:
  bool is_null() const {
    return static_cast<Tagged_t>(ptr()) == static_cast<Tagged_t>(kNullAddress);
  }

  // [map]: Contains a map which contains the object's reflective
  // information.
  DECL_GETTER(map, Map)
  inline void set_map(Map value);

  // This method behaves the same as `set_map` but marks the map transition as
  // safe for the concurrent marker (object layout doesn't change) during
  // verification.
  inline void set_map_safe_transition(Map value);

  inline ObjectSlot map_slot() const;

  // The no-write-barrier version.  This is OK if the object is white and in
  // new space, or if the value is an immortal immutable object, like the maps
  // of primitive (non-JS) objects like strings, heap numbers etc.
  inline void set_map_no_write_barrier(Map value,
                                       RelaxedStoreTag = kRelaxedStore);
  inline void set_map_no_write_barrier(Map value, ReleaseStoreTag);
  inline void set_map_safe_transition_no_write_barrier(
      Map value, RelaxedStoreTag = kRelaxedStore);
  inline void set_map_safe_transition_no_write_barrier(Map value,
                                                       ReleaseStoreTag);

  // Access the map using acquire load and release store.
  DECL_ACQUIRE_GETTER(map, Map)
  inline void set_map(Map value, ReleaseStoreTag);
  inline void set_map_safe_transition(Map value, ReleaseStoreTag);

  // Compare-and-swaps map word using release store, returns true if the map
  // word was actually swapped.
  inline bool release_compare_and_swap_map_word_forwarded(
      MapWord old_map_word, HeapObject new_target_object);

  // Initialize the map immediately after the object is allocated.
  // Do not use this outside Heap.
  inline void set_map_after_allocation(
      Map value, WriteBarrierMode mode = UPDATE_WRITE_BARRIER);

  // During garbage collection, the map word of a heap object does not
  // necessarily contain a map pointer.
  DECL_RELAXED_GETTER(map_word, MapWord)
  inline void set_map_word(Map map, RelaxedStoreTag);
  inline void set_map_word_forwarded(HeapObject target_object, RelaxedStoreTag);

  // Access the map word using acquire load and release store.
  DECL_ACQUIRE_GETTER(map_word, MapWord)
  inline void set_map_word(Map map, ReleaseStoreTag);
  inline void set_map_word_forwarded(HeapObject target_object, ReleaseStoreTag);

  // This method exists to help remove GetIsolate/GetHeap from HeapObject, in a
  // way that doesn't require passing Isolate/Heap down huge call chains or to
  // places where it might not be safe to access it.
  inline ReadOnlyRoots GetReadOnlyRoots() const;
  // This version is intended to be used for the isolate values produced by
  // i::GetPtrComprCageBase(HeapObject) function which may return nullptr.
  inline ReadOnlyRoots GetReadOnlyRoots(PtrComprCageBase cage_base) const;
  // This is slower, but safe to call during bootstrapping.
  inline ReadOnlyRoots EarlyGetReadOnlyRoots() const;

  // Whether the object is in the RO heap and the RO heap is shared, or in the
  // writable shared heap.
  V8_INLINE bool InAnySharedSpace() const;

  V8_INLINE bool InWritableSharedSpace() const;

  V8_INLINE bool InReadOnlySpace() const;

#define IS_TYPE_FUNCTION_DECL(Type) \
  V8_INLINE bool Is##Type() const;  \
  V8_INLINE bool Is##Type(PtrComprCageBase cage_base) const;
  HEAP_OBJECT_TYPE_LIST(IS_TYPE_FUNCTION_DECL)
  IS_TYPE_FUNCTION_DECL(HashTableBase)
  IS_TYPE_FUNCTION_DECL(SmallOrderedHashTable)
#undef IS_TYPE_FUNCTION_DECL

// Oddball checks are faster when they are raw pointer comparisons, so the
// isolate/read-only roots overloads should be preferred where possible.
#define IS_TYPE_FUNCTION_DECL(Type, Value, _)           \
  V8_INLINE bool Is##Type(Isolate* isolate) const;      \
  V8_INLINE bool Is##Type(LocalIsolate* isolate) const; \
  V8_INLINE bool Is##Type(ReadOnlyRoots roots) const;   \
  V8_INLINE bool Is##Type() const;
  ODDBALL_LIST(IS_TYPE_FUNCTION_DECL)
  IS_TYPE_FUNCTION_DECL(NullOrUndefined, , /* unused */)
#undef IS_TYPE_FUNCTION_DECL

#define DECL_STRUCT_PREDICATE(NAME, Name, name) \
  V8_INLINE bool Is##Name() const;              \
  V8_INLINE bool Is##Name(PtrComprCageBase cage_base) const;
  STRUCT_LIST(DECL_STRUCT_PREDICATE)
#undef DECL_STRUCT_PREDICATE

  V8_INLINE bool IsJSObjectThatCanBeTrackedAsPrototype() const;

  // Converts an address to a HeapObject pointer.
  static inline HeapObject FromAddress(Address address) {
    DCHECK_TAG_ALIGNED(address);
    return HeapObject(address + kHeapObjectTag);
  }

  // Returns the address of this HeapObject.
  inline Address address() const { return ptr() - kHeapObjectTag; }

  // Iterates over pointers contained in the object (including the Map).
  // If it's not performance critical iteration use the non-templatized
  // version.
  void Iterate(PtrComprCageBase cage_base, ObjectVisitor* v);

  template <typename ObjectVisitor>
  inline void IterateFast(PtrComprCageBase cage_base, ObjectVisitor* v);

  template <typename ObjectVisitor>
  inline void IterateFast(Map map, ObjectVisitor* v);

  template <typename ObjectVisitor>
  inline void IterateFast(Map map, int object_size, ObjectVisitor* v);

  // Iterates over all pointers contained in the object except the
  // first map pointer.  The object type is given in the first
  // parameter. This function does not access the map pointer in the
  // object, and so is safe to call while the map pointer is modified.
  // If it's not performance critical iteration use the non-templatized
  // version.
  void IterateBody(PtrComprCageBase cage_base, ObjectVisitor* v);
  void IterateBody(Map map, int object_size, ObjectVisitor* v);

  template <typename ObjectVisitor>
  inline void IterateBodyFast(PtrComprCageBase cage_base, ObjectVisitor* v);

  template <typename ObjectVisitor>
  inline void IterateBodyFast(Map map, int object_size, ObjectVisitor* v);

  // Returns true if the object contains a tagged value at given offset.
  // It is used for invalid slots filtering. If the offset points outside
  // of the object or to the map word, the result is UNDEFINED (!!!).
  V8_EXPORT_PRIVATE bool IsValidSlot(Map map, int offset);

  // Returns the heap object's size in bytes
  DECL_GETTER(Size, int)

  // Given a heap object's map pointer, returns the heap size in bytes
  // Useful when the map pointer field is used for other purposes.
  // GC internal.
  V8_EXPORT_PRIVATE int SizeFromMap(Map map) const;

  // Returns the field at offset in obj, as a read/write Object reference.
  // Does no checking, and is safe to use during GC, while maps are invalid.
  // Does not invoke write barrier, so should only be assigned to
  // during marking GC.
  inline ObjectSlot RawField(int byte_offset) const;
  inline MaybeObjectSlot RawMaybeWeakField(int byte_offset) const;
  inline CodeObjectSlot RawCodeField(int byte_offset) const;
  inline ExternalPointerSlot RawExternalPointerField(int byte_offset) const;

  DECL_CAST(HeapObject)

  // Return the write barrier mode for this. Callers of this function
  // must be able to present a reference to an DisallowGarbageCollection
  // object as a sign that they are not going to use this function
  // from code that allocates and thus invalidates the returned write
  // barrier mode.
  inline WriteBarrierMode GetWriteBarrierMode(
      const DisallowGarbageCollection& promise);

  // Dispatched behavior.
  void HeapObjectShortPrint(std::ostream& os);
#ifdef OBJECT_PRINT
  void PrintHeader(std::ostream& os, const char* id);
#endif
  DECL_PRINTER(HeapObject)
  EXPORT_DECL_VERIFIER(HeapObject)
#ifdef VERIFY_HEAP
  inline void VerifyObjectField(Isolate* isolate, int offset);
  inline void VerifySmiField(int offset);
  inline void VerifyMaybeObjectField(Isolate* isolate, int offset);

  // Verify a pointer is a valid HeapObject pointer that points to object
  // areas in the heap.
  static void VerifyHeapPointer(Isolate* isolate, Object p);
  static void VerifyCodePointer(Isolate* isolate, Object p);
#endif

  static inline AllocationAlignment RequiredAlignment(Map map);
  bool inline CheckRequiredAlignment(PtrComprCageBase cage_base) const;

  // Whether the object needs rehashing. That is the case if the object's
  // content depends on v8_flags.hash_seed. When the object is deserialized into
  // a heap with a different hash seed, these objects need to adapt.
  bool NeedsRehashing(InstanceType instance_type) const;
  bool NeedsRehashing(PtrComprCageBase cage_base) const;

  // Rehashing support is not implemented for all objects that need rehashing.
  // With objects that need rehashing but cannot be rehashed, rehashing has to
  // be disabled.
  bool CanBeRehashed(PtrComprCageBase cage_base) const;

  // Rehash the object based on the layout inferred from its map.
  template <typename IsolateT>
  void RehashBasedOnMap(IsolateT* isolate);

  // Layout description.
#define HEAP_OBJECT_FIELDS(V) \
  V(kMapOffset, kTaggedSize)  \
  /* Header size. */          \
  V(kHeaderSize, 0)

  DEFINE_FIELD_OFFSET_CONSTANTS(Object::kHeaderSize, HEAP_OBJECT_FIELDS)
#undef HEAP_OBJECT_FIELDS

  static_assert(kMapOffset == Internals::kHeapObjectMapOffset);

  using MapField = TaggedField<MapWord, HeapObject::kMapOffset>;

  inline Address GetFieldAddress(int field_offset) const;

 protected:
  // Special-purpose constructor for subclasses that have fast paths where
  // their ptr() is a Smi.
  enum class AllowInlineSmiStorage { kRequireHeapObjectTag, kAllowBeingASmi };
  inline HeapObject(Address ptr, AllowInlineSmiStorage allow_smi);

  OBJECT_CONSTRUCTORS(HeapObject, Object);

 private:
  enum class VerificationMode {
    kSafeMapTransition,
    kPotentialLayoutChange,
  };

  enum class EmitWriteBarrier {
    kYes,
    kNo,
  };

  template <EmitWriteBarrier emit_write_barrier, typename MemoryOrder>
  V8_INLINE void set_map(Map value, MemoryOrder order, VerificationMode mode);
};

OBJECT_CONSTRUCTORS_IMPL(HeapObject, Object)
CAST_ACCESSOR(HeapObject)

}  // namespace internal
}  // namespace v8

#include "src/objects/object-macros-undef.h"

#endif  // V8_OBJECTS_HEAP_OBJECT_H_


Set
   map
   filter
   set comprehension

Bag
   map
   filter
   product

Relation
   Join
   
———————————————
SQL Join = σ (r₁ × r₂)


———————————————
map f S
flat_map f S

map (λ s. s + 1) S


S₁ × S₂ ≔
flat_map (λ s₁.  map (λ s₂. 〈s₁, s₂〉) S₂    ) S₁

———————————
1                   a
2                   b
                     c

———————————
1                   a
                     b
                     c
{〈1, a〉
 〈1, b〉
 〈1, c〉}
———————————
2                   a
                     b
                     c
{〈2, a〉
 〈2, b〉
 〈2, c〉}
———————————
{〈1, a〉
 〈1, b〉
 〈1, c〉
 〈2, a〉
 〈2, b〉
 〈2, c〉}
———————————

———————————————
Relation r with columns〈id, name〉

σ_{λ〈i,n〉i > 100 ∧ |n| > 5} r
σ_{λ〈j, _〉j > 100} r

———————————————



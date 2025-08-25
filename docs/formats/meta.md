# Hush Metadata Files

## What are meta files?

When you're making a game, most of the time you will have resources that you load into the editor (sprites, models, music, textures, etc.), let's take a 3D model for example, you might want to import it into your game scene to create a visible character for your game, Hush will look at the model file and generate the right 3D representation of it, adding textures and materials where they should be, as well as creating new entities to follow your original file's hierarchy structure, all of those operations can get pretty expensive and slow the program down if we were to load all the model's data from scratch... This is why we, just as many engines out there, use **.meta** files! They allow us to do *a lot* of processing before the asset is loaded in your game by creating a relational model for your files at the time you import them to the project.

## What do they contain?

Well, the answer for this varies from file to file, as the data depends on the file extension, and whether or not your file references another resource, but generally you will find:

- The version of the metadata specification
- The ID of the file in question
- Feature Flags that determine the behavior of that file in the editor

## Format specification

Hush's meta files use JSON as their serialization format due to their parsing speed (yeah, we were surprised by this too!), compatibility with many object representations, and of course, their readability.

As said, a metafile will be different depending on the underlying file type it references, the following is an example using a simple PNG file:

```json
{
  "metadataVersion": 1,
  "id": 15963102,
  "mipMapFlags": 16711695,
  "sRGB": true
}
```

We have to reach a compromise between readability and the size and parsing speed, some properties might be serialized as flags.


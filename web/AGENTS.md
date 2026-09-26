<!-- BEGIN:nextjs-agent-rules -->

## Reglas del proyecto

- La llave de Supabase va en `SUPABASE_KEY`, nunca con prefijo `NEXT_PUBLIC_`.
  Esa llave puede insertar lecturas; si llega al navegador, cualquiera puede meter datos falsos.
- Todas las consultas a Supabase se hacen del lado del servidor.
- Las tablas están en el esquema `sbagua`, no en `public`.
<!-- END:nextjs-agent-rules -->

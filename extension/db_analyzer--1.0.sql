

CREATE FUNCTION backend_command(jsonb)
RETURNS void
AS 'extension', 'backend_command'
LANGUAGE C;
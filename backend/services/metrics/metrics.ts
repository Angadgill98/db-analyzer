import type { Pool } from "pg";



export class Metrics_Service{

    static:Static_metrics
    constructor(){
        this.static=new Static_metrics();
    }




}


class Static_metrics{


    constructor(){

    }

    GetBasicDbInfo(){
        return`
            SELECT
                current_database() AS database_name,
                current_user AS current_user,
                version() AS postgres_version;
        `;
    
    }
    
    GetUserRolePermissionsQuery() {
        return `
            SELECT
                rolname,
                rolsuper,
                rolinherit,
                rolcreaterole,
                rolcreatedb,
                rolcanlogin,
                rolreplication,
                rolbypassrls
            FROM pg_roles
            WHERE rolname = current_user;
        `;
    
    }
    
    GetDatabasePermissionsQuery() {
        return `
            SELECT json_build_object(
                'connect', has_database_privilege(current_user, current_database(), 'CONNECT'),
                'create', has_database_privilege(current_user, current_database(), 'CREATE'),
                'temporary', has_database_privilege(current_user, current_database(), 'TEMPORARY')
            )
        `;
    }
    
    GetSchemaLevelInfoQuery() {
        return `
            SELECT json_agg(
                json_build_object(
                    'schema_name', nspname,
                    'usage', has_schema_privilege(current_user, nspname, 'USAGE'),
                    'create', has_schema_privilege(current_user, nspname, 'CREATE')
                )
            )
            FROM pg_namespace
        `;
    }
    
    GetQueryBuilder(){
        let base_query="SELECT "
        return base_query;
    }
    
    GetTableLevelInfoQuery() {
        return `
            SELECT json_agg(
                json_build_object(
                    'schema_name', n.nspname,
                    'table_name', c.relname,
                    'select', has_table_privilege(current_user, c.oid, 'SELECT'),
                    'insert', has_table_privilege(current_user, c.oid, 'INSERT'),
                    'update', has_table_privilege(current_user, c.oid, 'UPDATE'),
                    'delete', has_table_privilege(current_user, c.oid, 'DELETE'),
                    'truncate', has_table_privilege(current_user, c.oid, 'TRUNCATE')
                )
            )
            FROM pg_class c
            JOIN pg_namespace n ON n.oid = c.relnamespace
            WHERE c.relkind IN ('r', 'p')
        `;
    }
    
    GetColumnLevelInfoQuery() {
        return `
            SELECT json_agg(
                json_build_object(
                    'schema_name', n.nspname,
                    'table_name', c.relname,
                    'column_name', a.attname,
                    'select', has_column_privilege(current_user, c.oid, a.attname, 'SELECT'),
                    'insert', has_column_privilege(current_user, c.oid, a.attname, 'INSERT'),
                    'update', has_column_privilege(current_user, c.oid, a.attname, 'UPDATE'),
                    'references', has_column_privilege(current_user, c.oid, a.attname, 'REFERENCES')
                )
            )
            FROM pg_attribute a
            JOIN pg_class c ON c.oid = a.attrelid
            JOIN pg_namespace n ON n.oid = c.relnamespace
            WHERE c.relkind IN ('r', 'p')
              AND a.attnum > 0
              AND NOT a.attisdropped
        `;
    }
    
    AddSubQuery(builder_base_query:string,query:string,alias_name:string){
        let separator = builder_base_query.trimEnd().endsWith("SELECT") ? "" : ", ";
    
        return `${builder_base_query}${separator}(${query}) AS ${alias_name}`;
    }

    GetStaticDataQuery(){
        let builder = this.GetQueryBuilder();

        const db_info_query = this.GetBasicDbInfo();
        builder = this.AddSubQuery(builder, db_info_query, "DB_info");

        const user_role_query = this.GetUserRolePermissionsQuery();
        builder = this.AddSubQuery(builder, user_role_query, "User_Role");

        const database_permissions_query = this.GetDatabasePermissionsQuery();
        builder = this.AddSubQuery(builder, database_permissions_query, "Database_Permissions");

        const schema_info_query = this.GetSchemaLevelInfoQuery();
        builder = this.AddSubQuery(builder, schema_info_query, "Schema_Info");

        const table_info_query = this.GetTableLevelInfoQuery();
        builder = this.AddSubQuery(builder, table_info_query, "Table_Info");

        const column_info_query = this.GetColumnLevelInfoQuery();
        builder = this.AddSubQuery(builder, column_info_query, "Column_Info");

        return builder;
    }

    async ExecuteBuilderQuery(query: string, db: Pool) {
        try {
            const result = await db.query(query);

            return result.rows;
        } catch (error) {
            console.error("Operation: ExecuteBuilderQuery");
            console.error("Error:", error);

            return undefined;
        }
    }
}


class Dynamic_metric{
    
}